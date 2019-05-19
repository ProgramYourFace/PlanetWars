#include "Chunk.h"
#include "Plugin.h"
#include "RenderEngine.h"
#include "ComputeBuffer.h"
#include "ComputeVolume.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

using namespace Eigen;

ComputeBuffer* Chunk::maxTris = nullptr;

Chunk::Chunk(const Vector3i& size, ID3D11Device* device) : size(size)
{
	tris = nullptr;
	volume = new ComputeVolume(DXGI_FORMAT_R16G16B16A16_FLOAT,
		size.x(),
		size.y(),
		size.z(),
		nullptr,
		4 * sizeof(short),
		device);

	cs = new CRITICAL_SECTION();
	InitializeCriticalSection(cs);
}

Chunk::~Chunk()
{
	SAFE_DELETE(tris);
	SAFE_DELETE(volume);
	DeleteCriticalSection(cs);
	delete cs;
}

void Chunk::InitializeTris(const int & count, ID3D11Device* device)
{
	SAFE_DELETE(tris);
	if (count > 0)
	{
		tris = new ComputeBuffer(BufferType::Append,
			nullptr,
			count,
			TRIANGLE_BSIZE,
			device);
	}
	vertexCount = count * 3;
}

void Chunk::Lock()
{
	EnterCriticalSection(cs);
}

void Chunk::Unlock()
{
	LeaveCriticalSection(cs);
}

EXPORT Chunk* CreateChunk(Vector3i size)
{
	return new Chunk(Vector3i(max(size.x(), 2), max(size.y(), 2), max(size.z(), 2)), GetRenderEngine()->device);
}

EXPORT void LockChunk(Chunk* chunk)
{
	chunk->Lock();
}

EXPORT void UnlockChunk(Chunk* chunk)
{
	chunk->Unlock();
}

EXPORT void InitializeMaxVertexBuffer(int maxResolution)
{
	SAFE_DELETE(Chunk::maxTris);
	int cellCount = maxResolution - 1;
	Chunk::maxTris = new ComputeBuffer(BufferType::Append,
		nullptr,
		cellCount * cellCount * cellCount * 5 + cellCount * cellCount * 4 * 6,
		Chunk::TRIANGLE_BSIZE,
		GetRenderEngine()->device);
}