#pragma once
#include <Eigen/Dense>
#include "Resource.h"

struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11ComputeShader;
class ComputeBuffer;
typedef unsigned int UINT;

class ComputeShader : public Resource
{
public:
	ComputeShader(const void* byteCode, const UINT& byteCodeLength, ID3D11Device* device);
	void Dispatch(ID3D11DeviceContext* context, const UINT& threadGroupX, const UINT& threadGroupY, const UINT& threadGroupZ);
	void DispatchIndirect(ID3D11DeviceContext* context, const ComputeBuffer* indirectArgs);
	static Eigen::Vector3i GetDispatch3D8Args(Eigen::Vector3i size);
	~ComputeShader() override;

private:
	ID3D11ComputeShader* shader;
};