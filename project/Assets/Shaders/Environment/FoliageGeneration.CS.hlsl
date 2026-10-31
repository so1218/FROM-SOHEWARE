#include "Common/ShaderConstants.hlsli"
#include "Common/MathUtils.hlsli"

ConstantBuffer<FoliageGenerationData> gGenerationData : register(b0);
ConstantBuffer<TerrainSettings> gTerrainSettings : register(b1);

Texture2D<float> gHeightMap : register(t0);
Texture2D<float> gDensityMap : register(t1);
SamplerState gLinearSampler : register(s0);

// 生えている所だけを詰めて出力する
AppendStructuredBuffer<FoliageInstanceData> gOutputFoliage : register(u0);

static const uint kThreadsPerRow = 1024 * 64;

// ワールド座標 → 地形のハイトマップの UV
float2 CalculateTerrainUV(float worldX, float worldZ)
{
    float u = (worldX - gGenerationData.terrainCenter.x) / gGenerationData.terrainWidth + 0.5f;
    float v = (worldZ - gGenerationData.terrainCenter.y) / gGenerationData.terrainDepth + 0.5f;
    return float2(u, v);
}

// from を to に向ける回転
float4 QuatFromVectors(float3 from, float3 to)
{
    float cosTheta = dot(from, to);

    // 同じ向き・逆向きのときは cross が 0 になるので別に扱う
    if (cosTheta > 0.9999f)
    {
        return float4(0.0f, 0.0f, 0.0f, 1.0f);
    }
    if (cosTheta < -0.9999f)
    {
        return float4(1.0f, 0.0f, 0.0f, 0.0f);
    }

    return normalize(float4(cross(from, to), 1.0f + cosTheta));
}

float4 QuatFromAxisAngle(float3 axis, float angle)
{
    float s, c;
    sincos(angle * 0.5f, s, c);
    return float4(axis * s, c);
}

float4 QuatMultiply(float4 q1, float4 q2)
{
    return float4(
        q1.w * q2.x + q1.x * q2.w + q1.y * q2.z - q1.z * q2.y,
        q1.w * q2.y - q1.x * q2.z + q1.y * q2.w + q1.z * q2.x,
        q1.w * q2.z + q1.x * q2.y - q1.y * q2.x + q1.z * q2.w,
        q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z
    );
}

[numthreads(64, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint instanceIndex = DTid.y * kThreadsPerRow + DTid.x;
    if (instanceIndex >= gGenerationData.maxInstancesPerChunk)
    {
        return;
    }

    // 格子状に並べてから、各点をランダムにずらして規則的に見えないように
    float gridSpacing = gGenerationData.gridSpacing;
    float2 snappedCenter = floor(gGenerationData.terrainCenter / gridSpacing) * gridSpacing;
    uint gridCountX = max((uint) ceil(gGenerationData.terrainWidth / gridSpacing), 1);
    uint gridX = instanceIndex % gridCountX;
    uint gridZ = instanceIndex / gridCountX;

    float2 gridPosition = snappedCenter
        - float2(gGenerationData.terrainWidth, gGenerationData.terrainDepth) * 0.5f
        + float2(gridX, gridZ) * gridSpacing;

    float2 jitter = Hash22(gridPosition) * (gridSpacing * 0.5f);
    float worldX = gridPosition.x + jitter.x;
    float worldZ = gridPosition.y + jitter.y;

    float2 terrainUV = CalculateTerrainUV(worldX, worldZ);
    if (any(terrainUV < 0.0f) || any(terrainUV > 1.0f))
    {
        return;
    }

    // 密度マップが 0.5 以上の所にだけ生やす (境目でまばらにならないよう 0 か 1 に)
    float density = step(0.5f, gDensityMap.SampleLevel(gLinearSampler, terrainUV, 0).r);
    if (Hash12(gridPosition) >= density)
    {
        return;
    }

    float worldY = (gHeightMap.SampleLevel(gLinearSampler, terrainUV, 0).r - 0.5f) * gTerrainSettings.maxHeight;

    // 周りの高さの差から地面の法線を求める
    float texelSize = gTerrainSettings.texelSize;
    float heightLeft = gHeightMap.SampleLevel(gLinearSampler, terrainUV + float2(-texelSize, 0.0f), 0).r;
    float heightRight = gHeightMap.SampleLevel(gLinearSampler, terrainUV + float2(texelSize, 0.0f), 0).r;
    float heightDown = gHeightMap.SampleLevel(gLinearSampler, terrainUV + float2(0.0f, texelSize), 0).r;
    float heightUp = gHeightMap.SampleLevel(gLinearSampler, terrainUV + float2(0.0f, -texelSize), 0).r;
    float3 terrainNormal = normalize(float3(
        (heightLeft - heightRight) * gTerrainSettings.maxHeight,
        2.0f * gTerrainSettings.cellSize,
        (heightDown - heightUp) * gTerrainSettings.maxHeight));

    // 斜面に完全に沿わせると寝てしまうので、真上寄りに傾ける
    float3 upVector = float3(0.0f, 1.0f, 0.0f);
    float4 alignRotation = QuatFromVectors(upVector, normalize(lerp(upVector, terrainNormal, 0.3f)));

    // 向きをばらばらにする
    float randomAngle = Hash12(float2(worldX * 1.3f, worldZ * 2.7f)) * PI * 2.0f;
    float4 rotation = QuatMultiply(alignRotation, QuatFromAxisAngle(upVector, randomAngle));

    float scale = lerp(gGenerationData.minScale, gGenerationData.maxScale, Hash12(float2(worldZ, worldX)));

    FoliageInstanceData foliage;
    foliage.posAndScale = float4(worldX, worldY, worldZ, scale);
    foliage.rotationQuat = rotation;
    gOutputFoliage.Append(foliage);
}
