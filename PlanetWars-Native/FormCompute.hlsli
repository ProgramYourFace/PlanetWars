cbuffer formParams : register(b0)
{
    float4x4 _voxelMatrix;
    uint3 _viewStart;
    uint3 _viewRange;
}

RWTexture3D<half4> _voxels : register(u0);

[numthreads(8, 8, 8)]
void main(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= _viewRange.x || id.y >= _viewRange.y || id.z >= _viewRange.z)
        return;
    
    uint3 index = id + _viewStart;
    float3 pos = mul(_voxelMatrix, float4(index, 1.0)).xyz;
    _voxels[index] = form(_voxels[index], pos);
}