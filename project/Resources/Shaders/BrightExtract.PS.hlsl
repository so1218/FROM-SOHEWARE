#include "FullScreenQuad.hlsli"
#include "ShaderConstants.hlsli"

Texture2D tex : register(t0);
SamplerState samLinear : register(s0);

ConstantBuffer<BrightExtractSettings> gBrightExtractSettings : register(b0);

float4 main(VSOutput input) : SV_TARGET
{
    float3 color = tex.Sample(samLinear, input.uv).rgb;

    float3 extractColor = max(color - gBrightExtractSettings.threshold, 0.0f);

    return float4(extractColor * gBrightExtractSettings.intensity, 1.0f);
}