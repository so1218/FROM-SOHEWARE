#ifndef GRASS_FIRE_COMMON_HLSLI
#define GRASS_FIRE_COMMON_HLSLI

#include "Common/ShaderConstants.hlsli"

// ワールドの XZ から地図の UV を求める
float2 CalculateGrassFireUV(float2 worldXZ, float2 areaCenter, float2 areaSize)
{
    return (worldXZ - areaCenter) / max(areaSize, float2(0.001f, 0.001f)) + 0.5f;
}

bool IsInsideGrassFireArea(float2 fireUV)
{
    return all(fireUV >= 0.0f) && all(fireUV <= 1.0f);
}

#endif
