#include "Plugin.h"
#include "ComputeBuffer.h"
#include <math.h>

ComputeBuffer::ComputeBuffer(const BufferType& type, const void* data, const int& count, const int& stride, ID3D11Device* device, const bool& sharable)
{
	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.ByteWidth = count * stride;
	size = bufferDesc.ByteWidth;
	if (type == BufferType::Constant)
	{
		bufferDesc.ByteWidth = (int)ceilf((float)bufferDesc.ByteWidth / 16.0f) * 16;
		if (bufferDesc.ByteWidth > D3D11_REQ_CONSTANT_BUFFER_ELEMENT_COUNT)
			LOG("Constant buffer is too large");
		bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bufferDesc.MiscFlags = 0;
		bufferDesc.StructureByteStride = 0;
		bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	}
	else
	{
		bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
		switch (type)
		{
		case BufferType::IndirectArgs:
			bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS | D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
			break;
		case BufferType::Raw:
			bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
			break;
		default:
			bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
			break;
		}
		bufferDesc.StructureByteStride = stride;
		bufferDesc.Usage = D3D11_USAGE_DEFAULT;
		bufferDesc.CPUAccessFlags = 0;
		if(sharable)
			bufferDesc.BindFlags |= D3D11_RESOURCE_MISC_SHARED;
	}

	if (data == nullptr)
	{
		HCHK(device->CreateBuffer(&bufferDesc, NULL, &buffer))
	}
	else
	{
		D3D11_SUBRESOURCE_DATA subData;
		ZeroMemory(&subData, sizeof(subData));
		subData.pSysMem = data;
		HCHK(device->CreateBuffer(&bufferDesc, &subData, &buffer))
	}

	if (type == BufferType::Constant)
	{
		uav = nullptr;
		srv = nullptr;
	}
	else
	{
		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
		ZeroMemory(&uavDesc, sizeof(uavDesc));
		if(type == BufferType::Raw || type == BufferType::IndirectArgs)
			uavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		else
			uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = count;
		switch (type)
		{
		case BufferType::Append:
			uavDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_APPEND;
			break;
		case BufferType::Count:
			uavDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_COUNTER;
			break;
		case BufferType::Raw:
			uavDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;
			break;
		case BufferType::IndirectArgs:
			uavDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;
			break;
		default:
			uavDesc.Buffer.Flags = 0;
			break;
		}

		HCHK(device->CreateUnorderedAccessView(buffer, &uavDesc, &uav))

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory(&srvDesc, sizeof(srvDesc));
		if (type == BufferType::Raw || type == BufferType::IndirectArgs)
		{
			srvDesc.BufferEx.FirstElement = 0;
			srvDesc.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
			srvDesc.BufferEx.NumElements = count;
			srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
		}
		else
		{
			srvDesc.Buffer.FirstElement = 0;
			srvDesc.Buffer.NumElements = count;
			srvDesc.Format = DXGI_FORMAT_UNKNOWN;
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		}

		HCHK(device->CreateShaderResourceView(buffer, &srvDesc, &srv))
	}
}

void ComputeBuffer::Bind(ID3D11DeviceContext* context, const UINT& slot, const bool& write, const UINT& appendCounter)
{
	if (uav && srv)
	{
		if (write)
			context->CSSetUnorderedAccessViews(slot, 1, &uav, &appendCounter);
		else
			context->CSSetShaderResources(slot, 1, &srv);
	}
	else
		context->CSSetConstantBuffers(slot, 1, &buffer);
}

void ComputeBuffer::Write(ID3D11DeviceContext* context, void * data)
{
	D3D11_MAPPED_SUBRESOURCE res;
	ZeroMemory(&res, sizeof(res));
	HCHK(context->Map(buffer, 0, D3D11_MAP::D3D11_MAP_WRITE_DISCARD, 0, &res))
	memcpy(res.pData, data, size);
	context->Unmap(buffer, 0);
}

void ComputeBuffer::CopyCount(ID3D11DeviceContext* context, ComputeBuffer* source, ComputeBuffer* destination, UINT byteOffset)
{
	context->CopyStructureCount(destination->buffer, byteOffset, source->uav);
}

ComputeBuffer::~ComputeBuffer()
{
	//LOG("Delete Compute buffer = " + to_string((unsigned long)buffer));
	SAFE_RELEASE(uav);
	SAFE_RELEASE(srv);
	SAFE_RELEASE(buffer);
}
