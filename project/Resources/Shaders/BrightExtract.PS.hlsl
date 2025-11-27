#include "FullScreenQuad.hlsli"
#include "ShaderConstants.hlsli"

Texture2D tex : register(t0);
SamplerState samLinear : register(s0);

ConstantBuffer<BrightExtractSettings> gBrightExtractSettings : register(b0);

float4 main(VSOutput input) : SV_TARGET
{
    float3 color = tex.Sample(samLinear, input.uv).rgb;

    float brightness = dot(color, float3(0.299, 0.587, 0.114)); // 輝度計算

    float factor = saturate((brightness - gBrightExtractSettings.threshold) / (1.0 - gBrightExtractSettings.threshold));

    return float4(color * factor * gBrightExtractSettings.intensity, 1.0);
}