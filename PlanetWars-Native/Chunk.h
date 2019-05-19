#pragma once
#include "Resource.h"
#include <Eigen/Dense>

extern "C"
{
	typedef struct _RTL_CRITICAL_SECTION RTL_CRITICAL_SECTION;
	typedef RTL_CRITICAL_SECTION CRITICAL_SECTION;
}

class ComputeBuffer;
class ComputeVolume;
struct ID3D11Device;

class Chunk : Resource
{

public:
	Chunk(const Eigen::Vector3i& size, ID3D11Device* device);
	~Chunk() override;

	void InitializeTris(const int& count, ID3D11Device* device);
	void Lock();
	void Unlock();

	Eigen::Vector3i size;
	ComputeVolume * volume;
	ComputeBuffer * tris;
	int vertexCount;

	static const int TRIANGLE_BSIZE = sizeof(float) * 16;
	static ComputeBuffer * maxTris;
private:
	CRITICAL_SECTION* cs;
};