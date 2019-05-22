#include "FormParameters.hlsli"

cbuffer sphereParameters : register(b1)
{
	float _radius;
	float _strength;
}

half4 form(half4 voxel, float3 position)
{
	float l = length(position);
	half a = min(l - _radius, 0.0f);
	return half4(voxel.rgb, voxel.a + a * _strength);
}

#include "FormCompute.hlsli"