#include "FullScreenQuad.hlsli"
#include "ShaderConstants.hlsli"

Texture2D<float> gDepthTexture : register(t0); // シーンの深度
Texture2D<float> gShadowMap : register(t1); // シャドウマップ

SamplerState gSampler : register(s0);
SamplerComparisonState gShadowSampler : register(s1); // 影判定用

// 定数バッファ
ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<CombineSettings> gCombineSettings : register(b1); // パラメータ用

// Henyey-Greenstein 位相関数 (光の散乱)
float PhaseFunctionHG(float cosTheta, float g)
{
    float g2 = g * g;
    float denom = 1.0f + g2 - 2.0f * g * cosTheta;
    return (1.0f - g2) / (4.0f * 3.14159265f * pow(denom, 1.5f));
}

float4 main(VSOutput input) : SV_TARGET
{
    float depthVal = gDepthTexture.Sample(gSampler, input.uv);

    // 背景の場合は計算をスキップ
    if (depthVal >= 1.0f)
    {
        return float4(0.0f, 0.0f, 0.0f, 1.0f);
    }

    // ワールド座標を復元
    float clipX = input.uv.x * 2.0f - 1.0f;
    float clipY = (1.0f - input.uv.y) * 2.0f - 1.0f;
    float4 clipPos = float4(clipX, clipY, depthVal, 1.0f);
    float4 worldPos = mul(clipPos, gFrameData.invViewProj);
    worldPos /= worldPos.w;

    // レイマーチングの準備
    float3 rayVec = worldPos.xyz - gFrameData.cameraWorldPosition;
    float rayLength = length(rayVec);
    float3 rayDir = rayVec / max(rayLength, 0.0001f);

    // 最大距離とステップ数をパラメータから取得
    float marchLength = min(rayLength, gCombineSettings.volumetricFogMaxDistance);
    int steps = gCombineSettings.volumetricFogSteps;
    float stepSize = marchLength / max((float) steps, 1.0f); // 0割り防止
    
    // ディザリング
    float dither = frac(sin(dot(input.uv, float2(12.9898, 78.233))) * 43758.5453) * stepSize;
    float3 currentPos = gFrameData.cameraWorldPosition + (rayDir * dither);

    float3 volumetricIllumination = float3(0, 0, 0);
    float3 lightDir = normalize(gFrameData.mainLightDirection);
    float cosTheta = dot(rayDir, lightDir);

    // 散乱係数(g値)をパラメータから取得
    float phase = PhaseFunctionHG(cosTheta, gCombineSettings.volumetricFogScatteringG);

    // レイマーチング・ループ
    for (int i = 0; i < steps; ++i)
    {
        float4 shadowCoord = mul(float4(currentPos, 1.0f), gFrameData.lightViewProj);
        shadowCoord.xyz /= shadowCoord.w;
        float2 shadowUV = shadowCoord.xy * float2(0.5f, -0.5f) + 0.5f;

        float shadowVisibility = 1.0f;
            
            // 範囲外チェック
        if (shadowUV.x >= 0.0f && shadowUV.x <= 1.0f && shadowUV.y >= 0.0f && shadowUV.y <= 1.0f)
        {
                // SampleCmpLevelZeroを使う
            float compareDepth = shadowCoord.z - 0.001f;
            shadowVisibility = gShadowMap.SampleCmpLevelZero(gShadowSampler, shadowUV, compareDepth);
            
        }

            // フォグの濃さをパラメータから取得
        float density = gCombineSettings.volumetricFogDensity;
            
        volumetricIllumination += density * shadowVisibility * phase * gFrameData.mainLightColor.rgb * stepSize;

        currentPos += rayDir * stepSize;
    }

    return float4(volumetricIllumination, 1.0f);
}