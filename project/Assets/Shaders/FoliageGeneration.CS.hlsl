#include "ShaderConstants.hlsli"

ConstantBuffer<FoliageGenerationData> gGenerationData : register(b0);
ConstantBuffer<TerrainSettings> gTerrainSettings : register(b1);

Texture2D<float> gHeightMap : register(t0);
Texture2D<float> gDensityMap : register(t1);
SamplerState gLinearSampler : register(s0);

AppendStructuredBuffer<FoliageInstanceData> gOutputFoliage : register(u0);

static const uint kThreadsPerRow = 1024 * 64;


// ワールド座標から地形全体の0-1 UVへのマッピング
float2 CalculateTerrainUV(float x, float z)
{
    float u = (x - gGenerationData.terrainCenter.x) / gGenerationData.terrainWidth + 0.5f;
    // DirectXのテクスチャV軸（上が0, 下が1）とワールドZ軸の向きを一致させる場合、反転が必要になるケースがあります
    float v = (z - gGenerationData.terrainCenter.y) / gGenerationData.terrainDepth + 0.5f;
    
    // ★ 地形シェーダーの uvTransform (xy: Scale, zw: Offset) を考慮する場合はここに乗算・加算します
    // (通常デフォルトは float4(1, 1, 0, 0) です)
    return float2(u, v);
}

// 疑似乱数 (Hash)
float Hash12(float2 p)
{
    float3 p3 = frac(float3(p.xyx) * 0.1031f);
    p3 += dot(p3, p3.yzx + 33.33f);
    return frac((p3.x + p3.y) * p3.z);
}

float2 Hash22(float2 p)
{
    float3 p3 = frac(float3(p.xyx) * float3(0.1031f, 0.1030f, 0.0973f));
    p3 += dot(p3, p3.yzx + 33.33f);
    return frac((p3.xx + p3.yz) * p3.zy);
}

// 2つのベクトルから回転クォータニオンを生成する関数
float4 QuatFromVectors(float3 u, float3 v)
{
    float cosTheta = dot(u, v);
    if (cosTheta > 0.9999f)
        return float4(0, 0, 0, 1);
    if (cosTheta < -0.9999f)
        return float4(1, 0, 0, 0); // 180度反転
    
    float3 axis = cross(u, v);
    float4 q = float4(axis, 1.0f + cosTheta);
    return normalize(q);
}

// 軸と角度からクォータニオンを生成する関数
float4 QuatFromAxisAngle(float3 axis, float angle)
{
    float s, c;
    sincos(angle * 0.5f, s, c);
    return float4(axis * s, c);
}

// クォータニオンの乗算
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
        return;

// =========================================================
    // 1. 密度マップから「現在の植物の密度」を取得
    // =========================================================
    float density = gDensityMap.SampleLevel(gLinearSampler, globalUV, 0).r;
    
    density = step(0.5f, density); // Maskの二値化による曖昧な境界ノイズの排除

    // この地点の乱数を生成
    float randomVal = Hash12(float2(baseWorldX, baseWorldZ));

    // 密度判定 (密度が低い、または乱数が上回ったら何も生えない)
    if (randomVal >= density)
        return;

    // =========================================================
    // 2. 高さ・法線計算
    // =========================================================
    float rawHeight = gHeightMap.SampleLevel(gLinearSampler, globalUV, 0).r;
    float worldY = (rawHeight - 0.5f) * gTerrainSettings.maxHeight;
    
    float offset = gTerrainSettings.texelSize;
    float hL = gHeightMap.SampleLevel(gLinearSampler, globalUV + float2(-offset, 0.0f), 0).r;
    float hR = gHeightMap.SampleLevel(gLinearSampler, globalUV + float2(offset, 0.0f), 0).r;
    float hD = gHeightMap.SampleLevel(gLinearSampler, globalUV + float2(0.0f, offset), 0).r;
    float hU = gHeightMap.SampleLevel(gLinearSampler, globalUV + float2(0.0f, -offset), 0).r;
    float3 terrainNormal = normalize(float3((hL - hR) * gTerrainSettings.maxHeight, 2.0f * gTerrainSettings.cellSize, (hD - hU) * gTerrainSettings.maxHeight));

    // =========================================================
    // 3. 植物の回転
    // =========================================================
    float3 upVector = float3(0, 1, 0);
    float3 plantNormal = normalize(lerp(upVector, terrainNormal, 0.3f));
    float4 alignQuat = QuatFromVectors(upVector, plantNormal);

    float randomAngle = Hash12(float2(worldX * 1.3f, worldZ * 2.7f)) * 3.14159265f * 2.0f;
    float4 randomYRotQuat = QuatFromAxisAngle(upVector, randomAngle);
    float4 finalQuat = QuatMultiply(alignQuat, randomYRotQuat);

    // =========================================================
    // 4. バッファへの追加 (Append)
    // =========================================================
    FoliageInstanceData inst = (FoliageInstanceData) 0;
    float randomScale = lerp(gGenerationData.minScale, gGenerationData.maxScale, Hash12(float2(worldZ, worldX)));
    inst.posAndScale = float4(worldX, worldY, worldZ, randomScale);
    inst.rotationQuat = finalQuat;
    
    float cJitter = lerp(0.7f, 1.0f, Hash12(float2(worldX * 2.0f, worldZ * 2.0f)));
    inst.colorVariation = float3(cJitter, cJitter, cJitter);

    // ★ 常にバインドされている u0 に書き込む (末尾のAppend処理を追加)
    gOutputFoliage.Append(inst);
}