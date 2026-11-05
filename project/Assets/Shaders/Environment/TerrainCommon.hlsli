#ifndef TERRAIN_COMMON_HLSLI
#define TERRAIN_COMMON_HLSLI

#include "Common/ShaderConstants.hlsli"

// ハイトマップの1ピクセルが地形の頂点1つにあたる。
// 頂点はちょうどピクセルの上にあるので、補間せずにそのまま読む
float LoadTerrainHeight(Texture2D<float> heightMap, TerrainSettings settings, int2 texel)
{
    int2 maxTexel = int2(settings.heightMapWidth, settings.heightMapHeight) - 1;
    float heightRatio = heightMap.Load(int3(clamp(texel, 0, maxTexel), 0));
    return (heightRatio - 0.5f) * settings.maxHeight;
}

float3 GetTerrainWorldPosition(TerrainSettings settings, int2 texel, float height)
{
    return settings.origin + float3(texel.x * settings.cellSize, height, texel.y * settings.cellSize);
}

#endif
