#pragma once

struct ID3D11Device;
struct ID3D11Buffer;
struct ID3D11DeviceContext;

class ComputeConstants
{
public:
	ComputeConstants(ID3D11Device* device);
	void Bind(ID3D11DeviceContext* context, const UINT& slot);
	~ComputeConstants();
};

