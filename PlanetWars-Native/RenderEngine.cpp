#include "Plugin.h"
#include "RenderEngine.h"
#include "Chunk.h"
#include "RenderTarget.h"
#include "ComputeShader.h"
#include "ComputeBuffer.h"
#include "ComputeVolume.h"
#include "IUnityGraphics.h"
#include "IUnityGraphicsD3D11.h"
#include "IUnityRenderingExtensions.h"
#include <mutex>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
using namespace Eigen;

RenderEngine::RenderEngine(IUnityInterfaces* unityInterface)
{
	IUnityGraphics* unityG = unityInterface->Get<IUnityGraphics>();
	supportsRendering = unityG->GetRenderer() == kUnityGfxRendererD3D11;
	if (supportsRendering)
	{
		unityGraphics = unityInterface->Get<IUnityGraphicsD3D11>();
		unityDevice = unityGraphics->GetDevice();
		unityDevice->GetImmediateContext(&unityContext);
	}

	UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
	HCHK(D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, flags, NULL, NULL, D3D11_SDK_VERSION, &device, NULL, &context))
}

static const int oneIArgs[] = { 1,1,1 };
void RenderEngine::InitializeComputeResources(
	void* meshingCSByteCode,
	const int& meshingCSLength,
	void* skirtCSByteCode,
	const int& skirtCSLength,
	void* chunkVSByteCode,
	const int& chunkVSByteCodeLength,
	void* chunkPSByteCode,
	const int& chunkPSByteCodeLength)
{
	meshingConstCB = new ComputeBuffer(BufferType::Constant, nullptr, 1, sizeof(unsigned int) * 3, device);
	meshingCS = new ComputeShader(meshingCSByteCode, meshingCSLength, device);
	skirtCS = new ComputeShader(skirtCSByteCode, skirtCSLength, device);

	strokeConstants = new ComputeBuffer(BufferType::Constant, nullptr, sizeof(BrushStrokeParameters), 1, device);
	contextCSect = new CRITICAL_SECTION();
	InitializeCriticalSection(contextCSect);

	if (supportsRendering)
	{
		HCHK(device->CreateVertexShader(chunkVSByteCode, chunkVSByteCodeLength, nullptr, &chunkVS))
		HCHK(device->CreatePixelShader(chunkPSByteCode, chunkPSByteCodeLength, nullptr, &chunkPS))

		sceneConstants = new ComputeBuffer(BufferType::Constant, nullptr, sizeof(SceneShaderConstants), 1, device);
		cameraConstants = new ComputeBuffer(BufferType::Constant, nullptr, sizeof(CameraShaderConstants), 1, device);
		modelConstants = new ComputeBuffer(BufferType::Constant, nullptr, sizeof(ModelShaderConstants), 1, device);

		queueCSect = new CRITICAL_SECTION();
		presentCSect = new CRITICAL_SECTION();
		InitializeCriticalSection(queueCSect);
		InitializeCriticalSection(presentCSect);

		immediateQueue = new DrawQueue();
		deferredQueue = new DrawQueue();

		// Render states
		D3D11_RASTERIZER_DESC rsdesc;
		memset(&rsdesc, 0, sizeof(rsdesc));
		rsdesc.FillMode = D3D11_FILL_SOLID;
		rsdesc.CullMode = D3D11_CULL_FRONT;
		rsdesc.DepthClipEnable = TRUE;
		device->CreateRasterizerState(&rsdesc, &solidRasterState);
		rsdesc.FillMode = D3D11_FILL_WIREFRAME;
		rsdesc.CullMode = D3D11_CULL_NONE;
		device->CreateRasterizerState(&rsdesc, &wireRasterState);

		D3D11_DEPTH_STENCIL_DESC dsdesc;
		memset(&dsdesc, 0, sizeof(dsdesc));
		dsdesc.DepthEnable = TRUE;
		dsdesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		dsdesc.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL;
		device->CreateDepthStencilState(&dsdesc, &depthState);

		D3D11_BLEND_DESC bdesc;
		memset(&bdesc, 0, sizeof(bdesc));
		bdesc.RenderTarget[0].BlendEnable = FALSE;
		bdesc.RenderTarget[0].RenderTargetWriteMask = 0xF;
		device->CreateBlendState(&bdesc, &blendState);

		context->RSSetState(solidRasterState);
		context->OMSetDepthStencilState(depthState, 0);
		context->OMSetBlendState(blendState, NULL, 0xFFFFFFFF);

		D3D11_BUFFER_DESC countDesc;
		ZeroMemory(&countDesc, sizeof(countDesc));
		countDesc.Usage = D3D11_USAGE_STAGING;
		countDesc.ByteWidth = sizeof(int);
		countDesc.StructureByteStride = countDesc.StructureByteStride;
		countDesc.BindFlags = 0;
		countDesc.MiscFlags = 0;
		countDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		HCHK(device->CreateBuffer(&countDesc, NULL, &countBuffer));
	}
}

void RenderEngine::ApplyForm(
	ComputeShader* computeShader,
	Chunk* chunk,
	ComputeBuffer* arguments,
	const BrushStrokeParameters& brushParameters)
{
	if (!chunk->volume)
		return;

	strokeConstants->Write(context, (void*)&brushParameters);
	strokeConstants->Bind(context, 0);
	if (arguments)
	{
		arguments->Bind(context, 1, false);
	}

	Vector3i dispatchArgs = ComputeShader::GetDispatch3D8Args(brushParameters.viewRange);

	chunk->volume->Bind(context, 0, true);
	computeShader->Dispatch(context,
		dispatchArgs.x(),
		dispatchArgs.y(),
		dispatchArgs.z());
}

void RenderEngine::SetSceneConstants(const SceneShaderConstants & scene)
{
	if (!supportsRendering)
		return;

	LockContext();
	sceneConstants->Write(context, (void*)&scene);
	UnlockContext();
}

void RenderEngine::SetWireframe(const bool & wireframe)
{
	if (!supportsRendering)
		return;

	LockContext();
	context->RSSetState(wireframe ? wireRasterState : solidRasterState);
	UnlockContext();
}

void RenderEngine::Draw(Chunk* chunk,
	const ModelShaderConstants& modelConstants,
	const Eigen::Matrix4f& viewProjection)
{
	if (!supportsRendering || !chunk->tris)
		return;

	DrawObject drawObj;
	drawObj.modelConstants = modelConstants;
	drawObj.boundsMin = Vector3f(0.0f, 0.0f, 0.0f);
	drawObj.boundsMax = Vector3f(chunk->size.x() - 1.0f, chunk->size.y() - 1.0f, chunk->size.z() - 1.0f);
	if (!drawObj.InFrustum(viewProjection))
		return;

	drawObj.drawResourceView = chunk->tris->srv;
	drawObj.vertexCount = chunk->vertexCount;
	drawObj.AddRef();

	EnterCriticalSection(queueCSect);
	deferredQueue->drawCalls.push_back(drawObj);
	LeaveCriticalSection(queueCSect);
}

void RenderEngine::Render(RenderPackage* package)
{
	if (!supportsRendering)
		return;

	LockContext();
	D3D11_VIEWPORT viewport;
	ZeroMemory(&viewport, sizeof(viewport));
	viewport.Width = (float)package->width;
	viewport.Height = (float)package->height;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	context->RSSetViewports(1, &viewport);

	context->VSSetShader(chunkVS, NULL, 0);
	context->PSSetShader(chunkPS, NULL, 0);

	cameraConstants->Write(context, (void*)&package->camera);
	ID3D11Buffer* constantBuffers[] = { sceneConstants->buffer, cameraConstants->buffer, modelConstants->buffer };
	context->VSSetConstantBuffers(0, 3, constantBuffers);
	
	const float clear[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	
	package->mutex->AcquireSync(0, INFINITE);
	context->OMSetRenderTargets(1, &package->colorView, package->depthView);
	context->ClearRenderTargetView(package->colorView, clear);
	context->ClearDepthStencilView(package->depthView, D3D11_CLEAR_DEPTH, 0.0f, 0);

	EnterCriticalSection(presentCSect);
	unsigned int drawCount = (unsigned int)immediateQueue->drawCalls.size();
	for (unsigned int i = 0; i < drawCount; i++)
	{
		DrawObject obj = immediateQueue->drawCalls[i];
		modelConstants->Write(context, (void*)&obj.modelConstants);
		context->VSSetShaderResources(0, 1, &obj.drawResourceView);
		context->DrawInstanced(obj.vertexCount, 1, 0, 0);
	}
	LeaveCriticalSection(presentCSect);

	SAFE_RELEASE(package->colorView);
	SAFE_RELEASE(package->depthView);
	context->Flush();
	package->mutex->ReleaseSync(0);
	SAFE_RELEASE(package->mutex);
	SAFE_DELETE(package);
	UnlockContext();
}

void RenderEngine::Present()
{
	EnterCriticalSection(presentCSect);
	immediateQueue->Release();
	DrawQueue* temp = immediateQueue;
	immediateQueue = deferredQueue;
	deferredQueue = temp;
	LeaveCriticalSection(presentCSect);
}

inline ID3D11Texture2D* GetViewTexture(ID3D11View* view)
{
	ID3D11Resource* res;
	view->GetResource(&res);
	ID3D11Texture2D* tex;
	res->QueryInterface(&tex);

	SAFE_RELEASE(res);
	return tex;
}

void RenderEngine::InjectRenderTarget(int unityRenderTarget, RenderTarget* nativeRenderTarget)
{
	if (!supportsRendering)
		return;

	IDXGIKeyedMutex* mutex = nativeRenderTarget->GetMutex(true);
	mutex->AcquireSync(0, INFINITE);
	if ((unityRenderTarget & 1) != 0)
	{
		ID3D11DepthStencilView* dsv;
		unityContext->OMGetRenderTargets(0, NULL, &dsv);
		ID3D11Texture2D* depthTexture = GetViewTexture(dsv);
		nativeRenderTarget->InjectDepthToUnity(unityContext, depthTexture);

		SAFE_RELEASE(depthTexture);
		SAFE_RELEASE(dsv);
	}
	if((unityRenderTarget & 2) != 0)
	{
		ID3D11RenderTargetView* rtv;
		unityContext->OMGetRenderTargets(1, &rtv, NULL);
		ID3D11Texture2D* colorTexture = GetViewTexture(rtv);
		nativeRenderTarget->InjectColorToUnity(unityContext, colorTexture);

		SAFE_RELEASE(colorTexture);
		SAFE_RELEASE(rtv);
	}
	mutex->ReleaseSync(0);
	SAFE_RELEASE(mutex);
}

void RenderEngine::LockContext()
{
	EnterCriticalSection(contextCSect);
}

void RenderEngine::UnlockContext()
{
	LeaveCriticalSection(contextCSect);
}

ComputeBuffer * RenderEngine::CreateConstantBuffer(void * data, const unsigned int & length)
{
	return new ComputeBuffer(BufferType::Constant, data, length, 1, device);
}

void RenderEngine::ComputeMesh(Chunk* chunk, const bool& skirts)
{
	if (!chunk->volume)
		return;

	Vector3i cellCount(chunk->size.x() - 1, chunk->size.y() - 1, chunk->size.z() - 1);
	Vector3i dispatchArgs = ComputeShader::GetDispatch3D8Args(cellCount);

	SAFE_DELETE(chunk->tris);

	meshingConstCB->Write(context, &cellCount);
	meshingConstCB->Bind(context, 0);
	Chunk::maxTris->Bind(context, 0, true, 0);
	chunk->volume->Bind(context, 0, false);
	meshingCS->Dispatch(context, dispatchArgs.x(), dispatchArgs.y(), dispatchArgs.z());
	if (skirts)
	{
		int maxDispatchArgs = max(dispatchArgs.x(), dispatchArgs.y());
		maxDispatchArgs = max(maxDispatchArgs, dispatchArgs.z());
		skirtCS->Dispatch(context, maxDispatchArgs, maxDispatchArgs, 1);
	}

	context->CopyStructureCount(countBuffer, 0, Chunk::maxTris->uav);

	D3D11_MAPPED_SUBRESOURCE res;
	ZeroMemory(&res, sizeof(res));
	int count = 0;
	HCHK(context->Map(countBuffer, 0, D3D11_MAP::D3D11_MAP_READ, 0, &res));
	memcpy(&count, res.pData, sizeof(int));
	context->Unmap(countBuffer, 0);

	chunk->InitializeTris(count, device);

	if (chunk->tris)
	{
		D3D11_BOX box;
		box.left = 0;
		box.right = count * Chunk::TRIANGLE_BSIZE;
		box.top = 0;
		box.bottom = 1;
		box.front = 0;
		box.back = 1;
		context->CopySubresourceRegion(chunk->tris->buffer, 0, 0, 0, 0, Chunk::maxTris->buffer, 0, &box);
	}
}

RenderEngine::~RenderEngine()
{
	SAFE_DELETE(Chunk::maxTris);
	SAFE_DELETE(meshingConstCB);
	SAFE_DELETE(meshingCS);
	SAFE_DELETE(skirtCS);
	SAFE_DELETE(strokeConstants);
	DeleteCriticalSection(contextCSect);
	SAFE_DELETE(contextCSect);

	if (supportsRendering)
	{
		SAFE_DELETE(immediateQueue);
		SAFE_DELETE(deferredQueue);
		SAFE_DELETE(sceneConstants);
		SAFE_DELETE(cameraConstants);
		SAFE_DELETE(modelConstants);
		SAFE_RELEASE(chunkVS);
		SAFE_RELEASE(chunkPS);
		SAFE_RELEASE(solidRasterState);
		SAFE_RELEASE(wireRasterState);
		SAFE_RELEASE(blendState);
		SAFE_RELEASE(depthState);
		SAFE_RELEASE(countBuffer);

		unityContext->Flush();
		SAFE_RELEASE(unityContext);

		DeleteCriticalSection(presentCSect);
		DeleteCriticalSection(queueCSect);
		SAFE_DELETE(presentCSect);
		SAFE_DELETE(queueCSect);
	}
	context->Flush();
	SAFE_RELEASE(context);
	SAFE_RELEASE(device);
}

EXPORT void* CreateConstantBuffer(void * data, int length)
{
	return GetRenderEngine()->CreateConstantBuffer(data, length);
}

EXPORT void ApplyForm(
	ComputeShader* computeShader,
	Chunk* chunk,
	ComputeBuffer* arguments,
	BrushStrokeParameters brushParameters)
{
	GetRenderEngine()->ApplyForm(computeShader, chunk, arguments, brushParameters);
}

EXPORT void ComputeMesh(Chunk* chunk, bool skirts)
{
	GetRenderEngine()->ComputeMesh(chunk, skirts);
}

EXPORT void SetSceneConstants(SceneShaderConstants scene)
{
	GetRenderEngine()->SetSceneConstants(scene);
}

EXPORT void SetWireframe(bool wireframe)
{
	GetRenderEngine()->SetWireframe(wireframe);
}

EXPORT void Draw(Chunk* chunk, ModelShaderConstants constants, Matrix4f viewProjection)
{
	GetRenderEngine()->Draw(chunk, constants, viewProjection);
}

EXPORT void Render(RenderPackage* package)
{
	GetRenderEngine()->Render(package);
}

EXPORT void Present()
{
	GetRenderEngine()->Present();
}


/*
EXPORT void LockContext()
{
	GetRenderEngine()->LockContext();
}

EXPORT void UnlockContext()
{
	GetRenderEngine()->UnlockContext();
}*/

EXPORT void CopyNativeToUnityBuffer(ComputeBuffer* native, ID3D11Resource* unity)
{
	ID3D11DeviceContext* context;
	GetRenderEngine()->device->GetImmediateContext(&context);
	context->CopyResource(unity, native->buffer);
	SAFE_RELEASE(context);
}

EXPORT void CopyNativeToUnityVolume(ComputeVolume* native, ID3D11Texture3D* unity)
{
	ID3D11DeviceContext* context;
	GetRenderEngine()->device->GetImmediateContext(&context);
	context->CopyResource(unity, native->texture);
	SAFE_RELEASE(context);
}

bool DrawObject::InFrustum(const Eigen::Matrix4f viewProjection)
{
	Matrix4f mvp = viewProjection * modelConstants.localToWorld;
	Vector4f corners[8] =
	{
		mvp * Vector4f(boundsMin.x(), boundsMin.y(), boundsMin.z(), 1.0f),
		mvp * Vector4f(boundsMax.x(), boundsMin.y(), boundsMin.z(), 1.0f),
		mvp * Vector4f(boundsMax.x(), boundsMax.y(), boundsMin.z(), 1.0f),
		mvp * Vector4f(boundsMin.x(), boundsMax.y(), boundsMin.z(), 1.0f),
		mvp * Vector4f(boundsMin.x(), boundsMin.y(), boundsMax.z(), 1.0f),
		mvp * Vector4f(boundsMax.x(), boundsMin.y(), boundsMax.z(), 1.0f),
		mvp * Vector4f(boundsMax.x(), boundsMax.y(), boundsMax.z(), 1.0f),
		mvp * Vector4f(boundsMin.x(), boundsMax.y(), boundsMax.z(), 1.0f),
	};

	int outs[6];
	ZeroMemory(outs, sizeof(outs));
	for (int i = 0; i < 8; i++)
	{
		Vector4f c = corners[i];
		float w = c.w();

		if (c.x() < -w) outs[0]++;
		if (c.x() > w) outs[1]++;

		if (c.y() < -w) outs[2]++;
		if (c.y() > w) outs[3]++;

		if (c.z() < 0.0f) outs[4]++;
		if (c.z() > w) outs[5]++;
	}

	if (outs[0] == 8 || outs[1] == 8 || outs[2] == 8 || outs[3] == 8 || outs[4] == 8 || outs[5] == 8)
		return false;
	return true;
}
