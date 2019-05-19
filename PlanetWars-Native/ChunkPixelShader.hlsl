struct v2f
{
    float4 vertex : SV_POSITION;
    float4 color : COLOR;
    float3 normal : NORMAL;
    float3 position : TEXCOORD0;
};

float4 main(v2f i) : SV_TARGET
{
    //float d = saturate(dot(-normalize(i.position), i.normal));
    return float4(i.color.rgb, 1.0);
}