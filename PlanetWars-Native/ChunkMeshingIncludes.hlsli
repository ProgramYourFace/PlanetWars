//INPUT
Texture3D<half4> _voxels : register(t0);

//CONSTANTS
cbuffer globals : register(b0)
{
    uint3 _chunkSize;
};

//OUTPUT
struct Triangle
{
    float3 pos[3];
    float4 col;
    float3 nrm;
};

AppendStructuredBuffer<Triangle> _triangles : register(u0);

static const float _level = 0.0;