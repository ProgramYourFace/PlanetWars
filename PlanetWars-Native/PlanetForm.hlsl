float hash(float n)
{
    return frac(sin(n) * 43758.5453);
}

float noise(float3 x)
{
    // The noise function returns a value in the range -1.0f -> 1.0f

    float3 p = floor(x);
    float3 f = frac(x);

    f = f * f * (3.0 - 2.0 * f);
    float n = p.x + p.y * 57.0 + 113.0 * p.z;

    return lerp(lerp(lerp(hash(n + 0.0), hash(n + 1.0), f.x),
                   lerp(hash(n + 57.0), hash(n + 58.0), f.x), f.y),
               lerp(lerp(hash(n + 113.0), hash(n + 114.0), f.x),
                   lerp(hash(n + 170.0), hash(n + 171.0), f.x), f.y), f.z);
}

float fractal(float3 x)
{
    float n = 0.0;
    float amp = 1.0;
    float ampSum = 0.0;
    [unroll(4)]
    for (int i = 0; i < 4; i++)
    {
        n += noise(x) * amp;
        x *= 2.0;
        ampSum += amp;
        amp *= 0.5;
    }
    return n / ampSum;
}

half4 form(half4 voxel, float3 position)
{
    float l = length(position);
    half s = (l - 0.75) * 10.0;
    float n = fractal(position * 5.0f) * 2.5;
    return half4(n / 2.0f, n / 2.0f, n / 2.0f, clamp(s + n, -1.0, 1.0));
}

#include "FormCompute.hlsli"