#include "FullScreenQuad.hlsli"
#include "ShaderConstants.hlsli"

Texture2D tex : register(t0);
SamplerState samLinear : register(s0);

ConstantBuffer<BrightExtractSettings> gBrightExtractSettings : register(b0);

float4 main(VSOutput input) : SV_TARGET
{
    float3 color = tex.Sample(samLinear, input.uv).rgb;

    float brightness = max(color.r, max(color.g, color.b));
    float threshold = gBrightExtractSettings.threshold;
    float knee = 0.1f; 

    // ソフトしきい値の計算
    float soft = brightness - threshold + knee;
    soft = clamp(soft, 0.0, 2.0 * knee);
    soft = soft * soft / (4.0 * knee + 0.00001);
    
    float contribution = max(soft, brightness - threshold) / max(brightness, 0.00001);
    float3 extractColor = color * contribution;

    extractColor = min(extractColor, float3(10.0f, 10.0f, 10.0f));
    return float4(extractColor * gBrightExtractSettings.intensity, 1.0f);
}