#pragma once

class ComputeBuffer;
class ComputeVolume;

struct VoxelBlock
{
public:
	VoxelBlock();

private:
	ComputeVolume * voxels;
	ComputeBuffer * meshArgs;
	ComputeBuffer * meshTriangles;
};