#include "ShaderConstants.hlsli"

ConstantBuffer<FoliageGenerationData> gGenerationData : register(b0);
ConstantBuffer<TerrainSettings> gTerrainSettings : register(b1);

Texture2D<float> gHeightMap : register(t0);
Texture2D<float> gDensityMap : register(t1);
SamplerState gLinearSampler : register(s0);

// NOTE: 描画時の DrawInstancedIndirect のために、有効なインスタンスのみを密にパックする
AppendStructuredBuffer<FoliageInstanceData> gOutputFoliage : register(u0);

static const uint kThreadsPerRow = 1024 * 64;

// ワールド座標から地形ハイトマップUVへのマッピング
// TODO: 地形マテリアル側にタイリング・オフセット(uvTransform)が追加された場合、ここで同期させる必要がある
float2 CalculateTerrainUV(float x, float z)
{
    float u = (x - gGenerationData.terrainCenter.x) / gGenerationData.terrainWidth + 0.5f;
    float v = (z - gGenerationData.terrainCenter.y) / gGenerationData.terrainDepth + 0.5f;
    return float2(u, v);
}

// 座標ベースの決定論的ハッシュ (ジッター、スケール、カラーのバリエーション用)
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

float4 QuatFromVectors(float3 u, float3 v)
{
    float cosTheta = dot(u, v);
    
    // 平行・反平行時のジンバルロック回避
    if (cosTheta > 0.9999f)
        return float4(0, 0, 0, 1);
    if (cosTheta < -0.9999f)
        return float4(1, 0, 0, 0);
    
    float3 axis = cross(u, v);
    return normalize(float4(axis, 1.0f + cosTheta));
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

    // グリッド配置 ＋ ジッター加算によるポアソンディスク分布の近似
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

    // 密度マップによる確率的カリング
    // 半端なアルファ値による境界のノイズ（まばらすぎる草）を避けるため、step関数で群生エリアを明確に二値化する
    float density = step(0.5f, gDensityMap.SampleLevel(gLinearSampler, globalUV, 0).r);
    if (Hash12(float2(baseWorldX, baseWorldZ)) >= density)
        return;

    // 中央差分による地形ローカル法線の算出
    float rawHeight = gHeightMap.SampleLevel(gLinearSampler, globalUV, 0).r;
    float worldY = (rawHeight - 0.5f) * gTerrainSettings.maxHeight;
    
    float offset = gTerrainSettings.texelSize;
    float hL = gHeightMap.SampleLevel(gLinearSampler, globalUV + float2(-offset, 0.0f), 0).r;
    float hR = gHeightMap.SampleLevel(gLinearSampler, globalUV + float2(offset, 0.0f), 0).r;
    float hD = gHeightMap.SampleLevel(gLinearSampler, globalUV + float2(0.0f, offset), 0).r;
    float hU = gHeightMap.SampleLevel(gLinearSampler, globalUV + float2(0.0f, -offset), 0).r;
    float3 terrainNormal = normalize(float3((hL - hR) * gTerrainSettings.maxHeight, 2.0f * gTerrainSettings.cellSize, (hD - hU) * gTerrainSettings.maxHeight));

    // 植物の重力屈性（太陽に向かって真っ直ぐ伸びる性質）の近似
    // 地形の法線に完全に沿わせると斜面で不自然に寝てしまうため、Y-Upベクトルとブレンドして姿勢を補正する
    float3 upVector = float3(0.0f, 1.0f, 0.0f);
    float3 plantNormal = normalize(lerp(upVector, terrainNormal, 0.3f));
    float4 alignQuat = QuatFromVectors(upVector, plantNormal);

    // Y軸まわりのランダム回転で不規則性を追加
    float randomAngle = Hash12(float2(worldX * 1.3f, worldZ * 2.7f)) * 3.14159265f * 2.0f;
    float4 randomYRotQuat = QuatFromAxisAngle(upVector, randomAngle);
    float4 finalQuat = QuatMultiply(alignQuat, randomYRotQuat);

    FoliageInstanceData inst = (FoliageInstanceData) 0;
    float randomScale = lerp(gGenerationData.minScale, gGenerationData.maxScale, Hash12(float2(worldZ, worldX)));
    inst.posAndScale = float4(worldX, worldY, worldZ, randomScale);
    inst.rotationQuat = finalQuat;
    
    float cJitter = lerp(0.7f, 1.0f, Hash12(float2(worldX * 2.0f, worldZ * 2.0f)));
    inst.colorVariation = float3(cJitter, cJitter, cJitter);

    gOutputFoliage.Append(inst);
}