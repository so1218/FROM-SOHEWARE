#include "FullScreenQuad.hlsli"
#include "ShaderConstants.hlsli"

Texture2D gSceneTexture : register(t0); // 元のシーン
Texture2D gBlurredBloom : register(t1); // ブラー済みBloomテクスチャ
SamplerState gSampler : register(s0);

ConstantBuffer<CombineSettings> gCombineSettings : register(b0);
struct PSInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

float4 main(VSOutput input) : SV_TARGET
{
    float4 sceneColor = gSceneTexture.Sample(gSampler, input.uv);
    float4 bloomColor = gBlurredBloom.Sample(gSampler, input.uv);

    float4 result;

    if (gCombineSettings.effectMode == 0)
    {
        float luminance = dot(bloomColor.rgb, float3(0.299, 0.587, 0.114));
        float3 haloColor = float3(1.0, 1.0, 1.0) * luminance * 1.5; // 白を強調
        haloColor = haloColor * gCombineSettings.bloomIntensity;
        result = sceneColor + float4(haloColor, 0.0);
    }
    else if (gCombineSettings.effectMode == 1)
    {
        // Neon: 元の色を強調した発光を加算
        result = sceneColor + bloomColor * gCombineSettings.bloomIntensity;
    }
    else if (gCombineSettings.effectMode == 2)
    { 
        // Overlayっぽい加算
        float3 overlay = 1.0 - (1.0 - bloomColor.rgb) * (1.0 - sceneColor.rgb);
        result.rgb = lerp(sceneColor.rgb, overlay, gCombineSettings.bloomIntensity);
    }
    else
    {
        // fallback: 通常のブルーム
        result = sceneColor + bloomColor * gCombineSettings.bloomIntensity;
    }

    result.rgb = saturate(result.rgb);
    result.a = sceneColor.a;
    
    return result;
}