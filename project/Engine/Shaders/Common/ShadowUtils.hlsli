#include "Common/ShaderConstants.hlsli"

static const float2 poissonDisk[16] =
{
    float2(-0.94201624, -0.39906216), float2(0.94558609, -0.76890725),
    float2(-0.094184101, -0.92938870), float2(0.34495938, 0.29387760),
    float2(-0.91588581, 0.45771432), float2(-0.81544232, -0.87912464),
    float2(-0.38277543, 0.27676845), float2(0.97484398, 0.75648379),
    float2(0.44323325, -0.97511554), float2(0.53742981, -0.47373420),
    float2(-0.26496911, -0.41893023), float2(0.79197514, 0.19090188),
    float2(-0.24188840, 0.99706507), float2(-0.81409955, 0.91437590),
    float2(0.19984126, 0.78641367), float2(0.14383161, -0.14100790)
};

float SampleSingleCascade(
    float3 worldPos, float3 normal, uint cascadeIndex, float3 lightDir,
    float shadowNormalBias, float shadowBias, float shadowSoftness,
    matrix cascadeLightViewProj,
    Texture2DArray<float> shadowMapArray,
    SamplerComparisonState shadowSampler)
{
    float NdotL = dot(normal, lightDir);
    
    // バイアス計算
    float biasScale = saturate(1.0f - NdotL);
    float worldNormalBias = shadowNormalBias * biasScale;
    float3 biasedWorldPos = worldPos + normal * worldNormalBias;

    // ライト空間への変換 
    float4 shadowCoord = mul(float4(biasedWorldPos, 1.0f), cascadeLightViewProj);
    float3 projCoords = shadowCoord.xyz / shadowCoord.w;

    projCoords.x = projCoords.x * 0.5f + 0.5f;
    projCoords.y = -projCoords.y * 0.5f + 0.5f;

    float currentDepth = projCoords.z - shadowBias;

    // 範囲外判定
    if (projCoords.z < 0.0f || projCoords.z > 1.0f ||
        projCoords.x < 0.0f || projCoords.x > 1.0f ||
        projCoords.y < 0.0f || projCoords.y > 1.0f)
    {
        return 1.0f; // 影なし
    }

    // テクセルサイズを定数化
    float2 texelSize = 1.0f / SHADOW_MAP_RESOLUTION;
    float softness = max(shadowSoftness, 1.0f);
    float shadow = 0.0f;

    // 16回のPCFサンプリング
    [unroll]
    for (int i = 0; i < 16; ++i)
    {
        float2 offset = poissonDisk[i] * texelSize * softness;
        shadow += shadowMapArray.SampleCmpLevelZero(
            shadowSampler,
            float3(projCoords.xy + offset, cascadeIndex),
            currentDepth
        );
    }

    return shadow * (1.0f / 16.0f);
}

// CSM全体の計算
float CalculateShadowCSM(
    float3 worldPos, float3 normal, float viewDepth, float3 lightDir,
    float shadowDensity, float4 cascadeSplits,
    float shadowNormalBias, float shadowBias, float shadowSoftness,
    float4x4 cascadeLightViewProj[MAX_CASCADE_COUNT],
    Texture2DArray<float> shadowMapArray,
    SamplerComparisonState shadowSampler)
{
    // 最遠判定：影の描画限界を超えた場合は処理をスキップ
    if (viewDepth > cascadeSplits[MAX_CASCADE_COUNT - 1])
        return 1.0f;

    float minShadow = 1.0f - saturate(shadowDensity);

    if (dot(normal, lightDir) <= 0.0f)
        return minShadow;

    // カスケードインデックスの判定
    uint cascadeIndex = 0;
    [unroll]
    for (uint i = 0; i < MAX_CASCADE_COUNT - 1; ++i)
    {
        if (viewDepth > cascadeSplits[i])
        {
            cascadeIndex = i + 1;
        }
    }

    // メインの影を取得
    float shadowVisibility = SampleSingleCascade(
        worldPos, normal, cascadeIndex, lightDir,
        shadowNormalBias, shadowBias, shadowSoftness,
        cascadeLightViewProj[cascadeIndex], shadowMapArray, shadowSampler
    );

    // 境界ブレンド処理
    if (cascadeIndex < MAX_CASCADE_COUNT - 1)
    {
        float nextSplitDist = cascadeSplits[cascadeIndex];
        float blendBand = 2.0f;
        float blendFactor = smoothstep(nextSplitDist - blendBand, nextSplitDist, viewDepth);

        if (blendFactor > 0.0f)
        {
            float nextShadowVisibility = SampleSingleCascade(
                worldPos, normal, cascadeIndex + 1, lightDir,
                shadowNormalBias, shadowBias, shadowSoftness,
                cascadeLightViewProj[cascadeIndex + 1], shadowMapArray, shadowSampler
            );
            shadowVisibility = lerp(shadowVisibility, nextShadowVisibility, blendFactor);
        }
    }

    return lerp(minShadow, 1.0f, shadowVisibility);
}