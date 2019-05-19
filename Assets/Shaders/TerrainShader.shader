Shader "Marching Cubes/Terrain"
{
	SubShader
	{
		Cull Back

		Pass
	{
		CGPROGRAM
		#pragma target 5.0
		#pragma vertex vert
		#pragma fragment frag

		#include "UnityCG.cginc"


	struct Triangle
	{
		float3 pos[3];
		float3 nrm;
		float3 col;
	};

	uniform StructuredBuffer<Triangle> _triangles;
	uniform float4x4 _model;

	struct v2f
	{
		float4 vertex : SV_POSITION;
		float3 normal : NORMAL;
		float3 color : TEXCOORD0;
	};

	v2f vert(uint id : SV_VertexID)
	{
		uint pid = id / 3;
		uint vid = id % 3;

		v2f o;
		o.vertex = mul(UNITY_MATRIX_VP, mul(_model, float4(_triangles[pid].pos[vid], 1)));
		o.normal = _triangles[pid].nrm;
		o.color = _triangles[pid].col;
		return o;
	}

	float4 frag(v2f i) : SV_Target
	{
		float d = max(dot(normalize(_WorldSpaceLightPos0.xyz), i.normal), 0);
		return float4(i.color, 1);
	}
		ENDCG
	}
	}
}