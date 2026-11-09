#include "Common/MathUtils.hlsli"

RWTexture3D<float4> gOutputNoise : register(u0);

static const uint kOctaveCount = 4;
static const float kBaseFrequency = 2.0f; 
static const float kWarpStrength = 0.15f;

// period ごとに繰り返すパーリン
float PeriodicPerlinNoise(float3 p, float period)
{
    float3 cell = floor(p);
    float3 local = p - cell;
    float3 weight = local * local * local * (local * (local * 6.0f - 15.0f) + 10.0f);

    float corners[8];
    [unroll]
    for (uint i = 0; i < 8; ++i)
    {
        float3 corner = float3(i & 1, (i >> 1) & 1, (i >> 2) & 1);
        corners[i] = dot(Hash33(fmod(cell + corner, period)), local - corner);
    }

    float x00 = lerp(corners[0], corners[1], weight.x);
    float x10 = lerp(corners[2], corners[3], weight.x);
    float x01 = lerp(corners[4], corners[5], weight.x);
    float x11 = lerp(corners[6], corners[7], weight.x);
    return lerp(lerp(x00, x10, weight.y), lerp(x01, x11, weight.y), weight.z);
}

// period ごとに繰り返すウォーリー
float PeriodicWorleyNoise(float3 p, float period)
{
    float3 cell = floor(p);
    float3 local = p - cell;
    float nearestDistance = 1.0f;

    for (int z = -1; z <= 1; ++z)
    {
        for (int y = -1; y <= 1; ++y)
        {
            for (int x = -1; x <= 1; ++x)
            {
                float3 offset = float3(x, y, z);
                float3 featurePoint = Hash33(fmod(cell + offset + period, period)) * 0.5f + 0.5f;
                nearestDistance = min(nearestDistance, length(offset + featurePoint - local));
            }
        }
    }
    return saturate(nearestDistance);
}

// オクターブを重ねたパーリン
float CalculateFractalPerlinNoise(float3 uvw)
{
    // 格子の向きが目立たないように少し歪ませる
    float3 warp = float3(
        PeriodicPerlinNoise(uvw * 2.0f, 2.0f),
        PeriodicPerlinNoise(uvw * 2.0f + 15.3f, 2.0f),
        PeriodicPerlinNoise(uvw * 2.0f + 31.1f, 2.0f));
    uvw += warp * kWarpStrength;

    // オクターブ同士で模様がそろわないようにずらす
    static const float3 kOctaveOffsets[kOctaveCount] =
    {
        float3(0.0f, 0.0f, 0.0f),
        float3(135.31f, 250.74f, 180.11f),
        float3(311.13f, 15.25f, 95.82f),
        float3(67.91f, 340.21f, 210.56f)
    };

    float noise = 0.0f;
    float amplitude = 0.5f;
    float totalAmplitude = 0.0f;
    float frequency = kBaseFrequency;
    for (uint i = 0; i < kOctaveCount; ++i)
    {
        noise += (PeriodicPerlinNoise(uvw * frequency + kOctaveOffsets[i], frequency) * 0.5f + 0.5f) * amplitude;
        totalAmplitude += amplitude;
        amplitude *= 0.5f;
        frequency *= 2.0f;
    }
    return noise / totalAmplitude;
}

[numthreads(8, 8, 8)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    uint width, height, depth;
    gOutputNoise.GetDimensions(width, height, depth);
    if (any(dispatchThreadId >= uint3(width, height, depth)))
        return;

    float3 uvw = float3(dispatchThreadId) / float3(width, height, depth);

    float perlin = smoothstep(0.1f, 0.9f, pow(CalculateFractalPerlinNoise(uvw), 1.2f));
    float largeWorley = 1.0f - PeriodicWorleyNoise(uvw * 4.0f, 4.0f);
    float smallWorley = 1.0f - PeriodicWorleyNoise(uvw * 8.0f, 8.0f);
    
    gOutputNoise[dispatchThreadId] = float4(perlin, largeWorley, smallWorley, 0.0f);
}
