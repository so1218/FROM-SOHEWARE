#include "Environment/TerrainCommon.hlsli"

ConstantBuffer<ShadowData> gShadowData : register(b8);
ConstantBuffer<CascadeConstant> gCascadeConstant : register(b9);
ConstantBuffer<TerrainSettings> gTerrainSettings : register(b10);

Texture2D<float> gHeightMap : register(t8);
StructuredBuffer<TerrainChunkData> gChunks : register(t9);

struct TerrainVSInput
{
    uint2 gridPosition : POSITION0;
};

float4 main(TerrainVSInput input, uint instanceID : SV_InstanceID) : SV_POSITION
{
    TerrainChunkData chunk = gChunks[instanceID];
    int2 texel = int2(chunk.startX, chunk.startZ) + int2(input.gridPosition);

    float height = LoadTerrainHeight(gHeightMap, gTerrainSettings, texel);
    float3 worldPosition = GetTerrainWorldPosition(gTerrainSettings, texel, height);

    return mul(float4(worldPosition, 1.0f), gShadowData.cascadeLightViewProj[gCascadeConstant.cascadeIndex]);
}
