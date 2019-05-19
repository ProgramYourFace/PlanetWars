#pragma once
#include "Resource.h"
#include <memory>

struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;
struct ID3D11ShaderResourceView;
struct ID3D11Texture2D;
struct ID3D11DeviceContext;
struct IDXGIKeyedMutex;
//namespace std { class mutex; }

struct RenderPackage
{
	int width, height;
	CameraShaderConstants camera;
	ID3D11RenderTargetView* colorView;
	ID3D11DepthStencilView* depthView;
	IDXGIKeyedMutex* mutex;
};

class RenderTarget : public Resource
{
public:
	RenderTarget(const int& width, const int& height, const bool& hdr, const bool& depthOnly, ID3D11Device* nativeDevice, ID3D11Device* unityDevice);
	void InjectDepthToUnity(ID3D11DeviceContext * context, ID3D11Texture2D * unityDepth);
	void InjectColorToUnity(ID3D11DeviceContext * context, ID3D11Texture2D * unityColor);
	IDXGIKeyedMutex* GetMutex(bool shared);
	~RenderTarget();

	int width, height;
	ID3D11RenderTargetView* colorView;
	ID3D11DepthStencilView* depthView;
private:
	ID3D11Texture2D* color;
	ID3D11Texture2D* colorShared;
	ID3D11Texture2D* depth;
	ID3D11Texture2D* depthShared;
};

