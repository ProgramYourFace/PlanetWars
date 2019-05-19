#pragma once
#include "Resource.h"

struct ID3D11Texture3D;
struct ID3D11ShaderResourceView;
struct ID3D11Device;
struct ID3D11DeviceContext;
typedef unsigned int UINT;

class ComputeVolume : public Resource
{
public:
	ComputeVolume(const DXGI_FORMAT& format, const UINT& width, const UINT& height, const UINT& depth, void* data,const int& pxStride, ID3D11Device* device);
	void Bind(ID3D11DeviceContext* context, const UINT& slot, const bool& write);
	~ComputeVolume() override;

public:
	ID3D11Texture3D* texture;
	ID3D11ShaderResourceView* srv;
	ID3D11UnorderedAccessView* uav;
};