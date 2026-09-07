#include "Common/FullScreenQuad.hlsli"
#include "Common/ShaderConstants.hlsli"

Texture2D gSceneTexture : register(t0); 
Texture2D gBloomTexture : register(t1); // Bloom用 (光のみボケ)
Texture2D gDoFTexture : register(t2); // DoF用 (全体ボケ)
Texture2D<float> gDepthTexture : register(t3);
Texture2D gVolumetricFogTexture : register(t4);
Texture2D gSSAOTexture : register(t5);
Texture2D gSSRTexture : register(t6); 

SamplerState gSampler : register(s0);
SamplerState gWrapSampler : register(s1);

ConstantBuffer<FinalCompositeSettings> gCompositeSettings : register(b0);
ConstantBuffer<FrameData> gFrameData : register(b1);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

// 深度リニア化関数
float LinearizeDepth(float d)
{
    float n = gFrameData.nearClip;
    float f = gFrameData.farClip;

    return (n * f) / (f - d * (f - n));
}

// トーンマッピング
float3 ACESFilm(float3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

float4 main(VSOutput input) : SV_TARGET
{
    float3 sceneColor = gSceneTexture.Sample(gSampler, input.uv).rgb;
    float3 combinedScene = sceneColor;

    // DoF の合成
    if (gCompositeSettings.enableDoF != 0)
    {
        float4 dofColor = gDoFTexture.Sample(gSampler, input.uv);
        combinedScene = lerp(combinedScene, dofColor.rgb, dofColor.a);
    }

    // SSAO の適用
    if (gCompositeSettings.enableSSAO != 0)
    {
        float ssao = gSSAOTexture.Sample(gSampler, input.uv).r;
        combinedScene *= ssao;
    }

    // SSR の加算
    if (gCompositeSettings.enableSSR != 0)
    {
        float4 ssrColor = gSSRTexture.Sample(gSampler, input.uv);
        combinedScene += ssrColor.rgb * ssrColor.a * gCompositeSettings.ssrIntensity;
    }

    // ブルーム成分の加算
    float3 bloomColor = gBloomTexture.Sample(gSampler, input.uv).rgb;
    float3 result = combinedScene + (bloomColor * gCompositeSettings.bloomIntensity);

    // ボリュメトリックフォグの合成
    if (gCompositeSettings.enableVolumetricFog != 0)
    {
        float4 vFogData = gVolumetricFogTexture.Sample(gSampler, input.uv);
        float3 vFogIllumination = vFogData.rgb;
        float vFogTransmittance = vFogData.a;

        result = result * vFogTransmittance + vFogIllumination;
    }

    // NaN のフォールバック処理
    if (any(isnan(result)))
    {
        result = float3(0.0f, 0.0f, 0.0f);
    }

    // トーンマッピング適用
    result = ACESFilm(clamp(result, 0.0f, 65504.0f));

    return float4(result, 1.0f);
}