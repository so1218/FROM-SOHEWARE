#ifndef FROXEL_COMMON_HLSLI
#define FROXEL_COMMON_HLSLI

#include "Common/CameraUtils.hlsli"

// フロクセルの奥行きは、近いほど細かくなるように指数で分ける
float GetFroxelNearDistance(float nearClip)
{
    return max(nearClip, kMinNearClip);
}

float GetFroxelFarDistance(float farClip, float maxDistance)
{
    return min(farClip, maxDistance);
}

// スライス番号から距離へ
float FroxelSliceToDistance(float slice, float sliceCount, float nearDistance, float farDistance)
{
    return nearDistance * pow(farDistance / nearDistance, slice / sliceCount);
}

// 距離から 0~1 の奥行き座標へ
float FroxelDistanceToDepthUV(float distance, float nearDistance, float farDistance)
{
    return log2(distance / nearDistance) / log2(farDistance / nearDistance);
}

// 画面の UV から、カメラが見ている方向
float3 GetViewRayDirection(float2 screenUV, float4x4 inverseViewProjection, float3 cameraPosition)
{
    float2 clipXY = float2(screenUV.x * 2.0f - 1.0f, (1.0f - screenUV.y) * 2.0f - 1.0f);
    float4 farPoint = mul(float4(clipXY, 1.0f, 1.0f), inverseViewProjection);
    return normalize(farPoint.xyz / farPoint.w - cameraPosition);
}

#endif
