#include "Plugin.h"
#include "ComputeShader.h"
#include "ComputeBuffer.h"
#include "DataReader.h"
#include "RenderEngine.h"

using namespace Eigen;

ComputeShader::ComputeShader(const void* byteCode, const UINT& byteCodeLength, ID3D11Device* device)
{
	HCHK(device->CreateComputeShader(byteCode, byteCodeLength, nullptr, &shader))
}

void ComputeShader::Dispatch(ID3D11DeviceContext* context, const UINT& threadGroupX, const UINT& threadGroupY, const UINT& threadGroupZ)
{
	context->CSSetShader(shader, NULL, NULL);
	context->Dispatch(threadGroupX, threadGroupY, threadGroupZ);
}

void ComputeShader::DispatchIndirect(ID3D11DeviceContext* context, const ComputeBuffer* indirectArgs)
{
	context->CSSetShader(shader, NULL, NULL);
	context->DispatchIndirect(indirectArgs->buffer, 0);
}

Vector3i ComputeShader::GetDispatch3D8Args(Vector3i size)
{
	return Vector3i((UINT)ceilf((float)max(size.x(), 0) / 8.0f),
		(UINT)ceilf((float)max(size.y(), 0) / 8.0f),
		(UINT)ceilf((float)max(size.z(), 0) / 8.0f));
}

ComputeShader::~ComputeShader()
{
	shader->Release();
}

EXPORT void* CreateComputeShader(void* byteCode, UINT byteCodeLength)
{
	return new ComputeShader(byteCode, byteCodeLength, GetRenderEngine()->device);
}