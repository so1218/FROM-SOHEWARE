#include "ShaderConstants.hlsli"

ConstantBuffer<PebbleGenerationData> gGenerationData : register(b0);
ConstantBuffer<TerrainSettings> gTerrainSettings : register(b1);
Texture2D<float> gHeightMap : register(t0);
Texture2D<float> gDensityMap : register(t1);
SamplerState gLinearSampler : register(s0);
RWStructuredBuffer<PebbleInstanceData> gOutputPebble : register(u0);

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
    
    // 範囲外チェック
    if (any(globalUV < 0.0f) || any(globalUV > 1.0f))
    {
        gOutputPebble[instanceIndex] = (PebbleInstanceData) 0;
        return;
    }

    // 密度マップによる生成判定
    float density = gDensityMap.SampleLevel(gLinearSampler, globalUV, 0).r;
    if (Hash12(float2(baseWorldX, baseWorldZ)) > density)
    {
        gOutputPebble[instanceIndex] = (PebbleInstanceData) 0;
        return;
    }

    // =========================================================
    // 1. 地形シェーダーと100%同一の高さ計算
    // =========================================================
    float rawHeight = gHeightMap.SampleLevel(gLinearSampler, globalUV, 0).r;
    
    // 地形頂点シェーダーとまったく同じリマップ式
    float localY = (rawHeight - 0.5f) * gTerrainSettings.maxHeight;
    
    // 地形自体のワールド座標(Y軸移動)がある場合、ここに加算する
    // 例: float worldY = localY + gGenerationData.terrainPositionY;
    float worldY = localY;

    // =========================================================
    // 2. 地形シェーダーと100%同一の法線計算（斜面への自動適応）
    // =========================================================
    float offset = gTerrainSettings.texelSize;
    float hL = gHeightMap.SampleLevel(gLinearSampler, globalUV + float2(-offset, 0.0f), 0).r;
    float hR = gHeightMap.SampleLevel(gLinearSampler, globalUV + float2(offset, 0.0f), 0).r;
    float hD = gHeightMap.SampleLevel(gLinearSampler, globalUV + float2(0.0f, offset), 0).r;
    float hU = gHeightMap.SampleLevel(gLinearSampler, globalUV + float2(0.0f, -offset), 0).r;
    
    float dx = (hL - hR) * gTerrainSettings.maxHeight;
    float dz = (hD - hU) * gTerrainSettings.maxHeight;
    
    // 地形のローカル法線 (TerrainVSの式と同じ)
    float3 terrainNormal = normalize(float3(dx, 2.0f * gTerrainSettings.cellSize, dz));

    // =========================================================
    // 3. 回転クォータニオンの構築 (地形の法線に傾けつつ、Y軸ランダム回転)
    // =========================================================
    // (A) 真上 (0,1,0) から地形の法線ベクトルへ傾ける回転
    float4 alignQuat = QuatFromVectors(float3(0.0f, 1.0f, 0.0f), terrainNormal);

    // (B) Y軸まわりのランダム回転（小石の向きをばらけさせる）
    float randomAngle = Hash12(float2(worldX * 1.3f, worldZ * 2.7f)) * 3.14159265f * 2.0f;
    float4 randomYRotQuat = QuatFromAxisAngle(float3(0.0f, 1.0f, 0.0f), randomAngle);

    // 回転を合成 (ランダム回転したあと、地形の斜面に沿って傾ける)
    float4 finalQuat = QuatMultiply(alignQuat, randomYRotQuat);

    // =========================================================
    // 4. スケールと埋め込み（Embed）
    // =========================================================
    float randomScale = lerp(gGenerationData.minScale, gGenerationData.maxScale, Hash12(float2(worldZ, worldX)));
    float scaleX = lerp(gGenerationData.minAnisoScale.x, gGenerationData.maxAnisoScale.x, Hash12(float2(worldX * 1.1f, worldZ * 0.9f)));
    float scaleY = lerp(gGenerationData.minAnisoScale.y, gGenerationData.maxAnisoScale.y, Hash12(float2(worldX * 1.5f, worldZ * 1.2f)));
    float scaleZ = lerp(gGenerationData.minAnisoScale.z, gGenerationData.maxAnisoScale.z, Hash12(float2(worldX * 0.8f, worldZ * 1.7f)));
    
    // ★ 地面に小石の底を自然になじませる埋め込み量 (例: 小石の高さの30%地面に沈める)
    float embedRatio = 0.3f;
    
    float colorJitter = lerp(0.8f, 1.0f, Hash12(float2(worldX, worldZ)));

    PebbleInstanceData pebble = (PebbleInstanceData) 0;
    pebble.posAndScale = float4(worldX, worldY, worldZ, randomScale);
    pebble.rotationQuat = finalQuat;
    pebble.anisoAndEmbed = float4(scaleX, scaleY, scaleZ, embedRatio);
    pebble.colorVariation = float3(colorJitter, colorJitter, colorJitter);

    gOutputPebble[instanceIndex] = pebble;
}