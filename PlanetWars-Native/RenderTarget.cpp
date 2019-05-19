#include "Plugin.h"
#include "RenderEngine.h"
#include "RenderTarget.h"
#include <mutex>

RenderTarget::RenderTarget(const int& width, const int& height, const bool& hdr, const bool& depthOnly, ID3D11Device* nativeDevice, ID3D11Device* unityDevice) : width(width), height(height)
{

	//Shared Handles
	HANDLE sharedHandle = NULL;
	IDXGIResource* dxgiTexture = NULL;
	ID3D11Resource* sharedResource = NULL;

	D3D11_TEXTURE2D_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(texDesc));
	texDesc.Width = width;
	texDesc.Height = height;
	texDesc.ArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.CPUAccessFlags = 0;

	if (!depthOnly)
	{
		texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		texDesc.Format = hdr ? DXGI_FORMAT_R16G16B16A16_FLOAT : DXGI_FORMAT_R8G8B8A8_UNORM;
		HCHK(nativeDevice->CreateTexture2D(&texDesc, NULL, &color))
		HCHK(nativeDevice->CreateRenderTargetView(color, NULL, &colorView))

		//Render Target Shared Resource
		HCHK(color->QueryInterface(&dxgiTexture))
		HCHK(dxgiTexture->GetSharedHandle(&sharedHandle))
		HCHK(unityDevice->OpenSharedResource(sharedHandle, __uuidof(ID3D11Resource), (void**)&sharedResource));
		HCHK(sharedResource->QueryInterface(&colorShared));

		SAFE_RELEASE(sharedResource);
		SAFE_RELEASE(dxgiTexture);
	}

	texDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX;
	texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	texDesc.Format = DXGI_FORMAT_R32G8X24_TYPELESS;
	HCHK(nativeDevice->CreateTexture2D(&texDesc, NULL, &depth))

	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
	ZeroMemory(&descDSV, sizeof(descDSV));
	descDSV.Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;

	HCHK(nativeDevice->CreateDepthStencilView(depth, &descDSV, &depthView))

	//Depth Stencil shared resource
	HCHK(depth->QueryInterface(&dxgiTexture))
	HCHK(dxgiTexture->GetSharedHandle(&sharedHandle))
	HCHK(unityDevice->OpenSharedResource(sharedHandle, __uuidof(ID3D11Resource), (void**)&sharedResource))
	HCHK(sharedResource->QueryInterface(&depthShared))

	SAFE_RELEASE(sharedResource);
	SAFE_RELEASE(dxgiTexture);
}

void RenderTarget::InjectDepthToUnity(ID3D11DeviceContext * context, ID3D11Texture2D * unityDepth)
{
	D3D11_TEXTURE2D_DESC depthDesc;
	unityDepth->GetDesc(&depthDesc);
	if (width == depthDesc.Width && height == depthDesc.Height)
		context->CopyResource(unityDepth, depthShared);
}

void RenderTarget::InjectColorToUnity(ID3D11DeviceContext * context, ID3D11Texture2D * unityColor)
{
	D3D11_TEXTURE2D_DESC colorDesc;
	unityColor->GetDesc(&colorDesc);
	if (width == colorDesc.Width && height == colorDesc.Height)
		context->CopyResource(unityColor, colorShared);
}

IDXGIKeyedMutex * RenderTarget::GetMutex(bool shared)
{
	IDXGIKeyedMutex* mutex;
	if (shared)
		HCHK(depthShared->QueryInterface(&mutex))
	else
		HCHK(depth->QueryInterface(&mutex))
	return mutex;
}

RenderTarget::~RenderTarget()
{
	SAFE_RELEASE(colorView);
	SAFE_RELEASE(color);
	SAFE_RELEASE(colorShared);
	SAFE_RELEASE(depthView);
	SAFE_RELEASE(depth);
	SAFE_RELEASE(depthShared);
}

EXPORT void* CreateRenderTarget(int width, int height, bool hdr, bool depthOnly)
{
	return new RenderTarget(width, height, hdr, depthOnly, GetRenderEngine()->device, GetRenderEngine()->unityDevice);
}

EXPORT void* GetRenderPackage(RenderTarget* renderTarget, CameraShaderConstants cameraConstants)
{
	renderTarget->colorView->AddRef();
	renderTarget->depthView->AddRef();
	RenderPackage* package = new RenderPackage();
	package->camera = cameraConstants;
	package->colorView = renderTarget->colorView;
	package->depthView = renderTarget->depthView;
	package->width = renderTarget->width;
	package->height = renderTarget->height;
	package->mutex = renderTarget->GetMutex(false);
	return package;
}