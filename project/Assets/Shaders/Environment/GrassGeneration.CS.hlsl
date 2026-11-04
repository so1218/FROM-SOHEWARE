#include "Common/ShaderConstants.hlsli"
#include "Common/MathUtils.hlsli"

ConstantBuffer<GrassGenerationData> gGenerationData : register(b0);
ConstantBuffer<TerrainSettings> gTerrainSettings : register(b1);

Texture2D<float> gHeightMap : register(t0);
Texture2D<float> gDensityMap : register(t1);
SamplerState gLinearSampler : register(s0);

RWStructuredBuffer<GrassInstanceData> gOutputGrass : register(u0);

// Dispatch(1024, 1, 1) と numthreads(64, 1, 1) で 1 行
static const uint kThreadsPerRow = 1024 * 64;

// ワールドの XZ から地形全体の 0~1 の UV にする
float2 CalculateTerrainUV(float2 worldXZ)
{
    float2 terrainSize = float2(gGenerationData.terrainWidth, gGenerationData.terrainDepth);
    return (worldXZ - gGenerationData.terrainCenter) / terrainSize + 0.5f;
}

[numthreads(64, 1, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    uint instanceIndex = dispatchThreadId.y * kThreadsPerRow + dispatchThreadId.x;
    if (instanceIndex >= gGenerationData.maxGrassPerChunk)
        return;

    float gridSpacing = gGenerationData.gridSpacing;
    float2 terrainSize = float2(gGenerationData.terrainWidth, gGenerationData.terrainDepth);

    // 格子に揃えて置くので、作り直しても同じ場所には同じ草が生える
    float2 snappedCenter = floor(gGenerationData.terrainCenter / gridSpacing) * gridSpacing;
    uint gridCountX = max((uint) ceil(terrainSize.x / gridSpacing), 1);
    uint2 gridIndex = uint2(instanceIndex % gridCountX, instanceIndex / gridCountX);
    float2 gridPosition = snappedCenter - terrainSize * 0.5f + float2(gridIndex) * gridSpacing;

    // 格子のままだと並んで見えるので、マスの中でずらす
    float2 worldXZ = gridPosition + (Hash22(gridPosition) - 0.5f) * gridSpacing;
    float2 terrainUV = CalculateTerrainUV(worldXZ);

    // 地形の外と、密度マップの半分より薄いところには生やさない。
    // 生やさない場所も 0 で埋めて、前に作った草が残らないようにする
    bool isOutside = any(terrainUV < 0.0f) || any(terrainUV > 1.0f);
    if (isOutside || gDensityMap.SampleLevel(gLinearSampler, terrainUV, 0).r < 0.5f)
    {
        gOutputGrass[instanceIndex] = (GrassInstanceData) 0;
        return;
    }

    float worldY = (gHeightMap.SampleLevel(gLinearSampler, terrainUV, 0).r - 0.5f) * gTerrainSettings.maxHeight;

    // 同じ乱数を使うと幅と向きが連動するので、種をずらす
    float randomHeight = lerp(gGenerationData.minHeight, gGenerationData.maxHeight, Hash12(worldXZ.yx));
    float randomWidth = lerp(gGenerationData.minWidth, gGenerationData.maxWidth, Hash12(worldXZ));
    float randomRotationY = Hash12(worldXZ * 1.37f + 17.0f) * PI * 2.0f;

    GrassInstanceData grass;
    grass.posAndHeight = float4(worldXZ.x, worldY, worldXZ.y, randomHeight);
    grass.rotationAndWidth = float4(randomRotationY, randomWidth, 0.0f, 0.0f);
    gOutputGrass[instanceIndex] = grass;
}
