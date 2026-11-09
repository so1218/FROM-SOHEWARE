#include "Common/ShaderConstants.hlsli"
#include "Common/MathUtils.hlsli"
#include "Environment/TerrainCommon.hlsli"

ConstantBuffer<FoliageGenerationData> gGenerationData : register(b0);
Texture2D<float> gDensityMap : register(t1);
SamplerState gLinearSampler : register(s0);

// 生えたものだけを詰めて書く
AppendStructuredBuffer<FoliageInstanceData> gOutputFoliage : register(u0);

// Dispatch の X は 1024 グループまでにして、残りは Y に折り返す
static const uint kThreadsPerRow = 1024 * 64;
// 斜面にそのまま沿わせると寝てしまうので、この割合だけ傾ける
static const float kSlopeAlignment = 0.3f;

// 生やす範囲の中での UV。密度マップはこの範囲に貼られている
float2 CalculateAreaUV(float2 worldXZ)
{
    float2 areaSize = float2(gGenerationData.terrainWidth, gGenerationData.terrainDepth);
    return (worldXZ - gGenerationData.terrainCenter) / areaSize + 0.5f;
}

float4 MultiplyQuaternion(float4 q1, float4 q2)
{
    return float4(
        q1.w * q2.x + q1.x * q2.w + q1.y * q2.z - q1.z * q2.y,
        q1.w * q2.y - q1.x * q2.z + q1.y * q2.w + q1.z * q2.x,
        q1.w * q2.z + q1.x * q2.y - q1.y * q2.x + q1.z * q2.w,
        q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z);
}

[numthreads(64, 1, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    uint instanceIndex = dispatchThreadId.y * kThreadsPerRow + dispatchThreadId.x;
    if (instanceIndex >= gGenerationData.maxInstanceCount)
        return;

    // 格子に並べ、格子の中でずらして規則的に見えないようにする。
    // 格子は gridSpacing の倍数にそろえて、範囲を動かしても生える位置が変わらないようにする
    float gridSpacing = gGenerationData.gridSpacing;
    float2 areaSize = float2(gGenerationData.terrainWidth, gGenerationData.terrainDepth);
    uint gridCountX = max((uint) ceil(areaSize.x / gridSpacing), 1u);
    float2 gridOrigin = floor(gGenerationData.terrainCenter / gridSpacing) * gridSpacing - areaSize * 0.5f;
    float2 gridPosition = gridOrigin + float2(instanceIndex % gridCountX, instanceIndex / gridCountX) * gridSpacing;

    float2 worldXZ = gridPosition + (Hash22(gridPosition) - 0.5f) * gridSpacing;
    float2 areaUV = CalculateAreaUV(worldXZ);
    if (any(areaUV < 0.0f) || any(areaUV > 1.0f))
        return;

    if (gDensityMap.SampleLevel(gLinearSampler, areaUV, 0) < 0.5f)
        return;

    // 地面の傾きに少し合わせてから、Y 軸まわりにばらばらに回す。
    // 真上から少し傾けるだけなので、真上と逆向きになることはない
    float3 up = float3(0.0f, 1.0f, 0.0f);
    float3 alignedUp = normalize(lerp(up, CalculateTerrainNormal(GetNearestTerrainTexel(worldXZ)), kSlopeAlignment));
    float4 slopeRotation = normalize(float4(cross(up, alignedUp), 1.0f + alignedUp.y));

    float halfYaw = Hash12(worldXZ * float2(1.3f, 2.7f)) * PI;
    float4 yawRotation = float4(0.0f, sin(halfYaw), 0.0f, cos(halfYaw));

    float scale = lerp(gGenerationData.minScale, gGenerationData.maxScale, Hash12(worldXZ.yx));

    FoliageInstanceData foliage;
    foliage.posAndScale = float4(worldXZ.x, SampleTerrainHeight(worldXZ), worldXZ.y, scale);
    foliage.rotationQuat = MultiplyQuaternion(slopeRotation, yawRotation);
    gOutputFoliage.Append(foliage);
}
