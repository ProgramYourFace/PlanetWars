Texture3D<half4> image : register(t0);

half4 form(half4 voxel, float3 position)
{
    uint width, height, depth;
    image.GetDimensions(width, height, depth);
    position = position * 0.5 + 0.5;
    float3 p = position * float3(width, height, depth);
    return half4(position, (image.Load(uint4(p, 0)) * -2.0 + 1.0).a);
}

#include "FormCompute.hlsli"