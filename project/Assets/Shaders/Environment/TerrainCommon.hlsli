#ifndef TERRAIN_COMMON_HLSLI
#define TERRAIN_COMMON_HLSLI

#include "Common/ShaderConstants.hlsli"

// 地形の描画と影の描画で同じレジスタを使う
ConstantBuffer<TerrainSettings> gTerrainSettings : register(b10);
Texture2D<float> gHeightMap : register(t9);
StructuredBuffer<TerrainChunkData> gChunks : register(t10);

// 頂点がハイトマップのどのピクセルにあたるか
int2 GetTerrainTexel(uint2 gridPosition, uint instanceID)
{
    TerrainChunkData chunk = gChunks[instanceID];
    return int2(chunk.startX, chunk.startZ) + int2(gridPosition);
}

// 頂点はちょうどピクセルの上にあるので、補間せずにそのまま読む
float LoadTerrainHeight(int2 texel)
{
    int2 maxTexel = int2(gTerrainSettings.heightMapWidth, gTerrainSettings.heightMapHeight) - 1;
    float heightRatio = gHeightMap.Load(int3(clamp(texel, 0, maxTexel), 0));
    return (heightRatio - 0.5f) * gTerrainSettings.maxHeight;
}

float3 GetTerrainWorldPosition(int2 texel, float height)
{
    return gTerrainSettings.origin + float3(texel.x * gTerrainSettings.cellSize, height, texel.y * gTerrainSettings.cellSize);
}

#endif
