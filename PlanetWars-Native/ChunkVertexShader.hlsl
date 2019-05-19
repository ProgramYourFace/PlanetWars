struct Triangle
{
    float3 pos[3];
    float4 col;
    float3 nrm;
};

StructuredBuffer<Triangle> _triangles : register(t0);

struct v2f
{
    float4 vertex : SV_POSITION;
    float4 color : COLOR;
    float3 normal : NORMAL;
    float3 position : TEXCOORD0;
};

cbuffer scene : register(b0)
{
    float4 light;
}

cbuffer camera : register(b1)
{
    float4x4 viewProjection;
}

cbuffer model : register(b2)
{
    float4x4 localToWorld;
}

v2f main(uint id : SV_VertexID)
{
    uint pid = id / 3;
    uint vid = id % 3;

    Triangle t = _triangles[pid];
    
    v2f o;
    float4 wsPos = mul(localToWorld, float4(t.pos[vid], 1.0));
    o.position = wsPos.xyz;
    o.vertex = mul(viewProjection, wsPos);
    o.color = t.col;
    o.normal = normalize(mul(localToWorld, float4(t.nrm, 0.0)).xyz);
    return o;
}