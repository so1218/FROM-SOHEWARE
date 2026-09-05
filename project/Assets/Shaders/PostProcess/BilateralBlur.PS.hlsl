#include "Common/FullScreenQuad.hlsli"
#include "Common/ShaderConstants.hlsli"

ConstantBuffer<BilateralBlurSettings> gBilateralBlurSettings : register(b0);
ConstantBuffer<FrameData> gFrameData : register(b1);

Texture2D<float> gInputTexture : register(t0);
Texture2D<float> gDepthTexture : register(t1); 
Texture2D<float4> gNormalTexture : register(t2); 

SamplerState gClampSampler : register(s0);

// 深度リニア化
float LinearizeDepth(float depth, float nearClip, float farClip)
{
    return (nearClip * farClip) / (farClip - depth * (farClip - nearClip));
}

float3 GetViewPos(float2 uv, float depth)
{
    float x = uv.x * 2.0f - 1.0f;
    float y = (1.0f - uv.y) * 2.0f - 1.0f;
    float4 clipPos = float4(x, y, depth, 1.0f);
    float4 viewPos = mul(clipPos, gFrameData.invProjMatrix);
    return viewPos.xyz / viewPos.w;
}

float4 main(VSOutput input) : SV_TARGET
{
    // 中心ピクセルの情報を取得
    float centerDepth = gDepthTexture.SampleLevel(gClampSampler, input.uv, 0);
    
    // 背景ならそのまま白（影なし）
    if (centerDepth >= 1.0f)
        return float4(1.0f, 1.0f, 1.0f, 1.0f);

    float centerZ = LinearizeDepth(centerDepth, gFrameData.nearClip, gFrameData.farClip);
    float3 centerNormal = gNormalTexture.SampleLevel(gClampSampler, input.uv, 0).xyz;
    float centerColor = gInputTexture.SampleLevel(gClampSampler, input.uv, 0);

    float totalWeight = 1.0f;
    float totalColor = centerColor;

    // ガウスぼかしの重み
    float spatialWeights[5] = { 0.227027f, 0.1945946f, 0.1216216f, 0.054054f, 0.016216f };

    int blurRadius = 4; // 左右(上下)に4ピクセルずつサンプリング

    for (int i = -blurRadius; i <= blurRadius; ++i)
    {
        if (i == 0)
            continue; // 中心は計算済みなのでスキップ

        // サンプリングするUV座標を計算
        float2 offset = input.uv + (gBilateralBlurSettings.direction * gBilateralBlurSettings.texelSize * (float) i);

        // 周辺ピクセルの情報を取得
        float sampleColor = gInputTexture.SampleLevel(gClampSampler, offset, 0);
        float sampleDepth = gDepthTexture.SampleLevel(gClampSampler, offset, 0);
        float3 sampleNormal = gNormalTexture.SampleLevel(gClampSampler, offset, 0).xyz;
        float sampleZ = LinearizeDepth(sampleDepth, gFrameData.nearClip, gFrameData.farClip);
        
        // 重みの計算
        // 距離による重み（遠いピクセルほど影響を小さく）
        float spatialW = spatialWeights[abs(i)];

        // 深度による重み（段差が大きいと重みが0に近づく）
        float depthDiff = abs(centerZ - sampleZ);
        float depthW = 1.0f / (1.0f + (depthDiff * depthDiff) / (gBilateralBlurSettings.depthTolerance * gBilateralBlurSettings.depthTolerance + 0.0001f));

        // 法線による重み（面が向いている方向が違うと重みが0に近づく）
        float normalW = pow(max(dot(centerNormal, sampleNormal), 0.0f), gBilateralBlurSettings.normalTolerance);

        // 最終的な重みを掛け合わせる
        float weight = spatialW * depthW * normalW;

        totalColor += sampleColor * weight;
        totalWeight += weight;
    }

    // 重みの合計で割って平均化
    float finalColor = totalColor / totalWeight;

    return float4(finalColor, finalColor, finalColor, 1.0f);
}