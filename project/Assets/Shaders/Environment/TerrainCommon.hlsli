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

// 両隣の高さの差から求める。両隣は2マス離れている
float3 CalculateTerrainNormal(int2 texel)
{
    float heightLeft = LoadTerrainHeight(texel + int2(-1, 0));
    float heightRight = LoadTerrainHeight(texel + int2(1, 0));
    float heightBack = LoadTerrainHeight(texel + int2(0, -1));
    float heightFront = LoadTerrainHeight(texel + int2(0, 1));
    return normalize(float3(heightLeft - heightRight, 2.0f * gTerrainSettings.cellSize, heightBack - heightFront));
}

// 地形のメッシュと同じ三角形の上で補間した、ワールドの高さ。C++ の Terrain::GetHeightAt と同じ計算
float SampleTerrainHeight(float2 worldXZ)
{
    float2 grid = (worldXZ - gTerrainSettings.origin.xz) / gTerrainSettings.cellSize;
    int2 cell = int2(floor(grid));
    float2 local = grid - cell;

    float height00 = LoadTerrainHeight(cell);
    float height10 = LoadTerrainHeight(cell + int2(1, 0));
    float height01 = LoadTerrainHeight(cell + int2(0, 1));
    float height11 = LoadTerrainHeight(cell + int2(1, 1));

    float height = (local.x + local.y <= 1.0f)
        ? height00 + (height10 - height00) * local.x + (height01 - height00) * local.y
        : height11 + (height01 - height11) * (1.0f - local.x) + (height10 - height11) * (1.0f - local.y);
    return gTerrainSettings.origin.y + height;
}

// ワールドの位置に一番近い頂点
int2 GetNearestTerrainTexel(float2 worldXZ)
{
    return int2(round((worldXZ - gTerrainSettings.origin.xz) / gTerrainSettings.cellSize));
}

#endif
