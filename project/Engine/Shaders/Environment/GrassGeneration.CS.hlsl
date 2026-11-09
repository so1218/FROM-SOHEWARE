#include "Common/ShaderConstants.hlsli"
#include "Common/MathUtils.hlsli"
#include "Environment/TerrainCommon.hlsli"

ConstantBuffer<GrassGenerationData> gGenerationData : register(b0);
Texture2D<float> gDensityMap : register(t1);
SamplerState gLinearSampler : register(s0);

RWStructuredBuffer<GrassInstanceData> gOutputGrass : register(u0);

// Dispatch の X は 1024 グループまでにして、残りは Y に折り返す
static const uint kThreadsPerRow = 1024 * 64;

// 生やす範囲の中での UV。密度マップはこの範囲に貼られている
float2 CalculateAreaUV(float2 worldXZ)
{
    float2 areaSize = float2(gGenerationData.terrainWidth, gGenerationData.terrainDepth);
    return (worldXZ - gGenerationData.terrainCenter) / areaSize + 0.5f;
}

[numthreads(64, 1, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    uint instanceIndex = dispatchThreadId.y * kThreadsPerRow + dispatchThreadId.x;
    if (instanceIndex >= gGenerationData.maxGrassCount)
        return;

    float gridSpacing = gGenerationData.gridSpacing;
    float2 areaSize = float2(gGenerationData.terrainWidth, gGenerationData.terrainDepth);

    // 格子に揃えて置くので、作り直しても同じ場所には同じ草が生える
    uint gridCountX = max((uint) ceil(areaSize.x / gridSpacing), 1u);
    float2 gridOrigin = floor(gGenerationData.terrainCenter / gridSpacing) * gridSpacing - areaSize * 0.5f;
    float2 gridPosition = gridOrigin + float2(instanceIndex % gridCountX, instanceIndex / gridCountX) * gridSpacing;

    // 格子のままだと並んで見えるので、マスの中でずらす
    float2 worldXZ = gridPosition + (Hash22(gridPosition) - 0.5f) * gridSpacing;
    float2 areaUV = CalculateAreaUV(worldXZ);

    // 範囲の外と、密度マップの半分より薄いところには生やさない。
    // 生やさない場所も 0 で埋めて、前に作った草が残らないようにする
    bool isOutside = any(areaUV < 0.0f) || any(areaUV > 1.0f);
    if (isOutside || gDensityMap.SampleLevel(gLinearSampler, areaUV, 0) < 0.5f)
    {
        gOutputGrass[instanceIndex] = (GrassInstanceData) 0;
        return;
    }

    // 同じ乱数を使うと幅と向きが連動するので、種をずらす
    float randomHeight = lerp(gGenerationData.minHeight, gGenerationData.maxHeight, Hash12(worldXZ.yx));
    float randomWidth = lerp(gGenerationData.minWidth, gGenerationData.maxWidth, Hash12(worldXZ));
    float randomRotationY = Hash12(worldXZ * 1.37f + 17.0f) * PI * 2.0f;

    GrassInstanceData grass;
    grass.posAndHeight = float4(worldXZ.x, SampleTerrainHeight(worldXZ), worldXZ.y, randomHeight);
    grass.rotationAndWidth = float4(randomRotationY, randomWidth, 0.0f, 0.0f);
    gOutputGrass[instanceIndex] = grass;
}
