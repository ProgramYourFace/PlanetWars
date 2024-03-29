#include "FormParameters.hlsli"

cbuffer sphereParameters : register(b1)
{
	float _radius;
	float _addSign;
}

half4 form(half4 voxel, float3 position)
{
    float l = length(position);
	half a = l - _radius;
    return half4(voxel.rgb, _addSign * min(a, _addSign * voxel.a));
}

#include "FormCompute.hlsli"