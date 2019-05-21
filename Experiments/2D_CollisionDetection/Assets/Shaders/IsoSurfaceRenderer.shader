Shader "IsoSurfaceRenderer"
{
    Properties
    {
		_DotSize("Invert", Range(0,1)) = 0.1
		_Color ("Color", Color) = (1.0,1.0,1.0,1.0)
        _Voxels ("Voxels", 2D) = "black" {}
		[Toggle(INVERT)]
		_Invert("Invert", Float) = 0
    }
    SubShader
    {
        Tags { "RenderType"="Opaque" }
        LOD 100

        Pass
        {
            CGPROGRAM
            #pragma vertex vert
            #pragma fragment frag

            #include "UnityCG.cginc"
			#pragma shader_feature INVERT

            struct appdata
            {
                float4 vertex : POSITION;
                float2 uv : TEXCOORD0;
            };

            struct v2f
            {
				float2 uv : TEXCOORD0;
                float2 voxelUv : TEXCOORD1;
                float4 vertex : SV_POSITION;
            };

			fixed _DotSize;
			fixed4 _Color;
            sampler2D_float _Voxels;
			float4 _Voxels_TexelSize;

            v2f vert (appdata v)
            {
                v2f o;
                o.vertex = UnityObjectToClipPos(v.vertex);
				o.uv = v.uv;
				o.voxelUv = (v.uv * (1 - _Voxels_TexelSize.xy)) + _Voxels_TexelSize.xy * 0.5;
                return o;
            }

            fixed4 frag (v2f i) : SV_Target
            {
                float alpha = tex2D(_Voxels, i.voxelUv).r;
#if INVERT
				clip(alpha);
#else
				clip(-alpha);
#endif
				float2 uvPos = (_Voxels_TexelSize.zw - 1) * i.uv;
				float2 delta = uvPos - round(uvPos);

                return lerp(_Color, float4(0.5,0.05,.25, 1.0), step(length(delta), _DotSize));
            }
            ENDCG
        }
    }
}
