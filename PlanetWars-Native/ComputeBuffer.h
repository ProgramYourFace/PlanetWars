#pragma once
#include "Resource.h"

enum BufferType
{
	Structured,
	Raw,
	Append,
	Count,
	Constant,
	IndirectArgs
};

struct ID3D11Device;
struct ID3D11Buffer;
struct ID3D11DeviceContext;
struct ID3D11UnorderedAccessView;
struct ID3D11ShaderResourceView;
typedef unsigned int UINT;

class ComputeBuffer : public Resource
{
public:
	ComputeBuffer(const BufferType& type, const void* data, const int& count, const int& stride, ID3D11Device* device, const bool& sharable = false);
	void Bind(ID3D11DeviceContext* context, const UINT& slot, const bool& write = true, const UINT& appendCounter = NULL);
	void Write(ID3D11DeviceContext* context, void* data);
	~ComputeBuffer() override;

	static void CopyCount(ID3D11DeviceContext* context, ComputeBuffer* source, ComputeBuffer* destination, UINT byteOffset);
public:
	ID3D11Buffer* buffer = NULL;
	ID3D11UnorderedAccessView* uav = NULL;
	ID3D11ShaderResourceView* srv = NULL;

private:
	UINT size;
};

