#ifndef CAMERA_UTILS_HLSLI
#define CAMERA_UTILS_HLSLI

static const float kMinNearClip = 0.1f;

// 深度リニア化
float LinearizeDepth(float depth, float nearClip, float farClip)
{
    return (nearClip * farClip) / (farClip - depth * (farClip - nearClip));
}

// 画面座標＋深度からビュー空間座標を復元
float3 GetViewPos(float2 uv, float depth, float4x4 invProjMatrix)
{
    float x = uv.x * 2.0f - 1.0f;
    float y = (1.0f - uv.y) * 2.0f - 1.0f;
    float4 clipPos = float4(x, y, depth, 1.0f);
    float4 viewPos = mul(clipPos, invProjMatrix);
    return viewPos.xyz / viewPos.w;
}

#endif