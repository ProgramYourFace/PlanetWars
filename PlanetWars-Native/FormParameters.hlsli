#ifndef FORM_PARAMETERS
#define FORM_PARAMETERS
cbuffer formParams : register(b0)
{
	float4x4 _voxelMatrix;
	uint3 _viewStart;
	uint3 _viewRange;
	float3 _formBounds;
}

RWTexture3D<half4> _voxels : register(u0);
#endif