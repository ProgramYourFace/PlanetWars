#include "FormParameters.hlsli"

[numthreads(8, 8, 8)]
void main(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= _viewRange.x || id.y >= _viewRange.y || id.z >= _viewRange.z)
        return;
    
    uint3 index = id + _viewStart;
    float3 pos = mul(_voxelMatrix, float4(index, 1.0)).xyz;
	_voxels[index] = form(_voxels[index], pos);//TODO: Clamp at crazy values so values do not compound into crazyness
}