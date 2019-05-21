Shader "Skybox/Grid"
{
	Properties
	{
		_Threshold("Threshold", float) = 10.0
		_GridSize("GridSize", Float) = 1.0
		_MinorGridSize("Minor Grid Size", Int) = 5
		_AxisLineColor("Axis Line Color", Color) = (0, 0, 0, 1)
		_MajorLineColor("Major Line Color", Color) = (0.1, 0.1, 0.1, 1)
		_MinorLineColor("Minor Line Color", Color) = (0.25, 0.25, 0.25, 1)
		_BackgroundColor("Background Color", Color) = (1, 1, 1, 1)
	}

	CGINCLUDE

	#include "UnityCG.cginc"

	struct appdata
	{
		float4 position : POSITION;
		float3 texcoord : TEXCOORD0;
	};

	struct v2f
	{
		float3 texcoord : TEXCOORD0;
		float minorSize : TEXCOORD1;
	};

	float _Threshold;
	float _GridSize;
	float _MinorGridSize;
	half4 _AxisLineColor;
	half4 _MajorLineColor;
	half4 _MinorLineColor;
	half4 _BackgroundColor;

	v2f vert(appdata v, out float4 outpos : SV_POSITION)
	{
		v2f o;
		outpos = UnityObjectToClipPos(v.position);
		o.texcoord = v.texcoord;
		o.minorSize = _GridSize * max(round(min(unity_OrthoParams.x, unity_OrthoParams.y) / (_Threshold * _GridSize)), 1);
		//o.minorSize = _GridSize * max(round(sqrt(unity_OrthoParams.x * unity_OrthoParams.y / (_Threshold * _GridSize * _GridSize))), 1);
		return o;
	}

	half4 frag(v2f i, UNITY_VPOS_TYPE screenPos : VPOS) : COLOR
	{
		float2 recipScreenParams = 1.0 / _ScreenParams.xy;
		float2 recipOrthoParams = 1.0 / unity_OrthoParams.xy;

		float2 planeUv = (((screenPos.xy) * recipScreenParams) - 0.5) * 2;
		planeUv.y = -planeUv.y;
		planeUv *= unity_OrthoParams.xy;
		planeUv += _WorldSpaceCameraPos.xy;

		int2 gridIndex = round(planeUv / i.minorSize);
		float2 roundedUv = gridIndex * i.minorSize;
		roundedUv -= _WorldSpaceCameraPos.xy;
		roundedUv /= unity_OrthoParams.xy;
		roundedUv.y = -roundedUv.y;
		roundedUv = floor((roundedUv * 0.5 + 0.5) * _ScreenParams.xy);

		int2 edgeDelta = abs(roundedUv - floor(screenPos));
		int distance = min(edgeDelta.x, edgeDelta.y);

		if(distance == 0)
		{
			if(gridIndex.x == 0 && edgeDelta.x <= edgeDelta.y || gridIndex.y == 0 && edgeDelta.y <= edgeDelta.x)
			{
				return _AxisLineColor;
			}
			else if(round(gridIndex.x / _MinorGridSize) * _MinorGridSize  == gridIndex.x && edgeDelta.x <= edgeDelta.y || round(gridIndex.y / _MinorGridSize) * _MinorGridSize  == gridIndex.y && edgeDelta.y <= edgeDelta.x)
			{
				return _MajorLineColor;
			}
			return _MinorLineColor;
		}
		return _BackgroundColor;
	}
	ENDCG

	SubShader
	{
		Tags{ "RenderType" = "Skybox" "Queue" = "Background" }
			Pass
		{
			ZWrite Off
			Cull Off
			Fog { Mode Off }
			CGPROGRAM
			#pragma fragmentoption ARB_precision_hint_fastest
			#pragma vertex vert
			#pragma fragment frag
			ENDCG
		}
	}
}