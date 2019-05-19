half4 form(half4 voxel, float3 position)
{
    float l = length(position);
    half a = clamp((l - 1), -1.0, 1.0);
    return half4(position / l, a);
}

#include "FormCompute.hlsli"