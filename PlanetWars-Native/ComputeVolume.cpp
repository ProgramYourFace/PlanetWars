#include "Plugin.h"
#include "ComputeVolume.h"
#include "RenderEngine.h"

ComputeVolume::ComputeVolume(const DXGI_FORMAT& format, const UINT& width, const UINT& height, const UINT& depth, void* data,const int& pxStride, ID3D11Device* device)
{
	D3D11_TEXTURE3D_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(texDesc));
	texDesc.Width = width;
	texDesc.Height = height;
	texDesc.Depth = depth;
	texDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.Format = format;
	texDesc.MipLevels = 1;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.MiscFlags = 0;
	if (data)
	{
		D3D11_SUBRESOURCE_DATA d;
		d.pSysMem = data;
		d.SysMemPitch = pxStride * width;
		d.SysMemSlicePitch = d.SysMemPitch * height;
		HCHK(device->CreateTexture3D(&texDesc, &d, &texture))
	}
	else
		HCHK(device->CreateTexture3D(&texDesc, NULL, &texture))

	HCHK(device->CreateShaderResourceView(texture, NULL, &srv))
	HCHK(device->CreateUnorderedAccessView(texture, NULL, &uav))
}

void ComputeVolume::Bind(ID3D11DeviceContext* context, const UINT& slot, const bool& write)
{
	if (write)
		context->CSSetUnorderedAccessViews(slot, 1, &uav, NULL);
	else
		context->CSSetShaderResources(slot, 1, &srv);
}

ComputeVolume::~ComputeVolume()
{
	SAFE_RELEASE(srv);
	SAFE_RELEASE(uav);
	SAFE_RELEASE(texture);
}

EXPORT void* CreateVolume(int width, int height, int depth, void* data)
{
	return new ComputeVolume(DXGI_FORMAT_R16G16B16A16_FLOAT, width, height, depth, data, 8, GetRenderEngine()->device);
}

EXPORT void CSBindVolume(int slot, ComputeVolume* volume, bool write)
{

	if (write)
		GetRenderEngine()->context->CSSetUnorderedAccessViews(slot, 1, &volume->uav, 0);
	else
		GetRenderEngine()->context->CSSetShaderResources(slot, 1, &volume->srv);
}