#include "Common/ShaderConstants.hlsli"
#include "Common/MathUtils.hlsli"

ConstantBuffer<PebbleGenerationData> gGenerationData : register(b0);
ConstantBuffer<TerrainSettings> gTerrainSettings : register(b1);

Texture2D<float> gHeightMap : register(t0);
Texture2D<float> gDensityMap : register(t1);
SamplerState gLinearSampler : register(s0);

RWStructuredBuffer<PebbleInstanceData> gOutputPebble : register(u0);

static const uint kThreadsPerRow = 1024 * 64;

// ワールド空間から地形UV空間(0.0 - 1.0)へのマッピング
float2 CalculateTerrainUV(float x, float z)
{
    float u = (x - gGenerationData.terrainCenter.x) / gGenerationData.terrainWidth + 0.5f;
    float v = (z - gGenerationData.terrainCenter.y) / gGenerationData.terrainDepth + 0.5f;
    return float2(u, v);
}

// 2ベクトル間の最短回転を表すクォータニオンを算出
float4 QuatFromVectors(float3 u, float3 v)
{
    float cosTheta = dot(u, v);
    
    // 平行・反平行時の特異点(ジンバルロック)回避
    if (cosTheta > 0.9999f)
        return float4(0, 0, 0, 1);
    if (cosTheta < -0.9999f)
        return float4(1, 0, 0, 0);
    
    float3 axis = cross(u, v);
    float4 q = float4(axis, 1.0f + cosTheta);
    return normalize(q);
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
        return;

    // グリッドベースで基本位置を決定し、ジッターで散らす
    float gridSpacing = gGenerationData.gridSpacing;
    float2 snappedCenter = floor(gGenerationData.terrainCenter / gridSpacing) * gridSpacing;

    uint gridSizeX = max((uint) ceil(gGenerationData.terrainWidth / gridSpacing), 1);
    uint gridX = instanceIndex % gridSizeX;
    uint gridZ = instanceIndex / gridSizeX;
    float2 localPos = float2(gridX * gridSpacing, gridZ * gridSpacing);

    float offsetX = -gGenerationData.terrainWidth * 0.5f;
    float offsetZ = -gGenerationData.terrainDepth * 0.5f;

    float baseWorldX = snappedCenter.x + offsetX + localPos.x;
    float baseWorldZ = snappedCenter.y + offsetZ + localPos.y;

    float2 jitter = Hash22(float2(baseWorldX, baseWorldZ)) * (gridSpacing * 0.5f);
    float worldX = baseWorldX + jitter.x;
    float worldZ = baseWorldZ + jitter.y;
    
    float2 globalUV = CalculateTerrainUV(worldX, worldZ);
    
    if (any(globalUV < 0.0f) || any(globalUV > 1.0f))
    {
        gOutputPebble[instanceIndex] = (PebbleInstanceData) 0;
        return;
    }

    // 密度マップによるカリング
    float density = gDensityMap.SampleLevel(gLinearSampler, globalUV, 0).r;
    if (Hash12(float2(baseWorldX, baseWorldZ)) > density)
    {
        gOutputPebble[instanceIndex] = (PebbleInstanceData) 0;
        return;
    }
    
    // NOTE: 地形側の頂点シェーダーとハイト・法線の計算ロジックを完全に一致させること
    float rawHeight = gHeightMap.SampleLevel(gLinearSampler, globalUV, 0).r;
    float localY = (rawHeight - 0.5f) * gTerrainSettings.maxHeight;
    
    // TODO: 地形全体にY軸のワールドオフセットが入る場合はここで加算
    float worldY = localY;
    
    float offset = gTerrainSettings.texelSize;
    float hL = gHeightMap.SampleLevel(gLinearSampler, globalUV + float2(-offset, 0.0f), 0).r;
    float hR = gHeightMap.SampleLevel(gLinearSampler, globalUV + float2(offset, 0.0f), 0).r;
    float hD = gHeightMap.SampleLevel(gLinearSampler, globalUV + float2(0.0f, offset), 0).r;
    float hU = gHeightMap.SampleLevel(gLinearSampler, globalUV + float2(0.0f, -offset), 0).r;
    
    float dx = (hL - hR) * gTerrainSettings.maxHeight;
    float dz = (hD - hU) * gTerrainSettings.maxHeight;
    float3 terrainNormal = normalize(float3(dx, 2.0f * gTerrainSettings.cellSize, dz));

    // 地面の傾斜に追従させつつ、Y軸でランダムに回転させて不規則性を出す
    float4 alignQuat = QuatFromVectors(float3(0.0f, 1.0f, 0.0f), terrainNormal);
    float randomAngle = Hash12(float2(worldX * 1.3f, worldZ * 2.7f)) * 3.14159265f * 2.0f;
    float4 randomYRotQuat = QuatFromAxisAngle(float3(0.0f, 1.0f, 0.0f), randomAngle);
    float4 finalQuat = QuatMultiply(alignQuat, randomYRotQuat);

    // スケールの非均等化とカラーのジッター
    float randomScale = lerp(gGenerationData.minScale, gGenerationData.maxScale, Hash12(float2(worldZ, worldX)));
    float scaleX = lerp(gGenerationData.minAnisoScale.x, gGenerationData.maxAnisoScale.x, Hash12(float2(worldX * 1.1f, worldZ * 0.9f)));
    float scaleY = lerp(gGenerationData.minAnisoScale.y, gGenerationData.maxAnisoScale.y, Hash12(float2(worldX * 1.5f, worldZ * 1.2f)));
    float scaleZ = lerp(gGenerationData.minAnisoScale.z, gGenerationData.maxAnisoScale.z, Hash12(float2(worldX * 0.8f, worldZ * 1.7f)));
    float colorJitter = lerp(0.8f, 1.0f, Hash12(float2(worldX, worldZ)));
    
    // 接地感を出すための沈み込み量(30%)
    float embedRatio = 0.3f;
    
    PebbleInstanceData pebble = (PebbleInstanceData) 0;
    pebble.posAndScale = float4(worldX, worldY, worldZ, randomScale);
    pebble.rotationQuat = finalQuat;
    pebble.anisoAndEmbed = float4(scaleX, scaleY, scaleZ, embedRatio);
    pebble.colorVariation = float3(colorJitter, colorJitter, colorJitter);

    gOutputPebble[instanceIndex] = pebble;
}