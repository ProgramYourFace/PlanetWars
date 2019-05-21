#pragma once
#include <Eigen/Dense>
#include <vector>

struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct ID3D11InputLayout;
struct ID3D11ShaderResourceView;
struct ID3D11Buffer;
struct ID3D11RasterizerState;
struct ID3D11BlendState;
struct ID3D11DepthStencilState;
struct IUnityInterfaces;
struct IUnityGraphicsD3D11;
struct UnityRenderingExtCustomBlitParams;
struct RenderPackage;
class Chunk;
class RenderTarget;
class ComputeShader;
class ComputeBuffer;
class ComputeVolume;

extern "C"
{
	typedef struct _RTL_CRITICAL_SECTION RTL_CRITICAL_SECTION;
	typedef RTL_CRITICAL_SECTION CRITICAL_SECTION;
}

_declspec(align(16)) struct BrushStrokeParameters
{
	Eigen::Matrix4f voxelMatrix;
	Eigen::Vector3i viewStart;
	float pad;
	Eigen::Vector3i viewRange;
	float pad1;
	Eigen::Vector3f formBounds;
};

_declspec(align(16)) struct SceneShaderConstants
{
	Eigen::Vector4f light;
};

_declspec(align(16)) struct CameraShaderConstants
{
	Eigen::Matrix4f viewProjection;
};

_declspec(align(16)) struct ModelShaderConstants
{
	Eigen::Matrix4f localToWorld;
};

struct DrawObject
{
	int vertexCount;
	ID3D11ShaderResourceView* drawResourceView;
	ModelShaderConstants modelConstants;
	Eigen::Vector3f boundsMin;
	Eigen::Vector3f boundsMax;

	bool InFrustum(const Eigen::Matrix4f viewProjection);

	void AddRef()
	{
		if (drawResourceView) drawResourceView->AddRef();
	}

	void Release()
	{
		SAFE_RELEASE(drawResourceView);
	}
};

struct DrawQueue
{
	std::vector<DrawObject> drawCalls;

	~DrawQueue()
	{
		Release();
	}

	void AddRef()
	{
		unsigned int size = (unsigned int)drawCalls.size();
		for (unsigned int i = 0; i < size; i++)
			drawCalls[i].AddRef();
	}

	void Release()
	{
		unsigned int size = (unsigned int)drawCalls.size();
		for (unsigned int i = 0; i < size; i++)
			drawCalls[i].Release();

		drawCalls.shrink_to_fit();
		drawCalls.clear();
	}
};

class RenderEngine
{
public:
	RenderEngine(IUnityInterfaces* unityInterface);

	void InitializeComputeResources(
		void* meshingCSByteCode,
		const int& meshingCSLength,
		void* skirtCSByteCode,
		const int& skirtCSLength,
		void* chunkVSByteCode,
		const int& chunkVSByteCodeLength,
		void* chunkPSByteCode,
		const int& chunkPSByteCodeLength);
	void ApplyForm(
		ComputeShader* computeShader,
		Chunk* chunk,
		ComputeBuffer* arguments,
		const BrushStrokeParameters& brushParameters);
	void SetSceneConstants(const SceneShaderConstants& scene);
	void SetWireframe(const bool& wireframe);
	void Draw(Chunk* chunk, const ModelShaderConstants& modelConstants, const Eigen::Matrix4f& viewProjection);
	void Render(RenderPackage* package);
	void Present();
	void InjectRenderTarget(int unityRenderTarget, RenderTarget* nativeRenderTarget);

	void LockContext();
	void UnlockContext();
	ComputeBuffer* CreateConstantBuffer(void* data, const unsigned int& length);

	void ComputeMesh(Chunk* chunk, const bool& skirts);

	~RenderEngine();
public:
	bool supportsRendering;

	ID3D11Device* device;
	ID3D11DeviceContext* context;
	ID3D11Device* unityDevice;
	ID3D11DeviceContext* unityContext;
	IUnityGraphicsD3D11* unityGraphics;

private:
	ComputeBuffer* meshingConstCB;
	ComputeShader* meshingCS;
	ComputeShader* skirtCS;
	ComputeBuffer* strokeConstants;
	CRITICAL_SECTION* contextCSect;

	//Rendering
	//Shaders
	ID3D11VertexShader* chunkVS = NULL;
	ID3D11PixelShader* chunkPS = NULL;
	ID3D11RasterizerState* solidRasterState;
	ID3D11RasterizerState* wireRasterState;
	ID3D11BlendState* blendState;
	ID3D11DepthStencilState* depthState;
	ID3D11Buffer* countBuffer;
	ComputeBuffer* sceneConstants = NULL;
	ComputeBuffer* cameraConstants = NULL;
	ComputeBuffer* modelConstants = NULL;

	//Draw
	DrawQueue* immediateQueue;
	DrawQueue* deferredQueue;
	CRITICAL_SECTION* queueCSect;
	CRITICAL_SECTION* presentCSect;
};