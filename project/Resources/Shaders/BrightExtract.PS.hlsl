#include "FullScreenQuad.hlsli"

Texture2D tex : register(t0);
SamplerState samLinear : register(s0);

cbuffer BrightExtractSettings : register(b0)
{
    float threshold; // 0.7 ～ 1.0 くらいが目安
    float intensity; // 強調度
    float2 _padding;
};

float4 main(VSOutput input) : SV_TARGET
{
    float3 color = tex.Sample(samLinear, input.uv).rgb;

    float brightness = dot(color, float3(0.299, 0.587, 0.114)); // 輝度計算

    float factor = saturate((brightness - threshold) / (1.0 - threshold));

    return float4(color * factor * intensity, 1.0);
}