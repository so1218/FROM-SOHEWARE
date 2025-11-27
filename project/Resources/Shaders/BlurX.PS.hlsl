#include "FullScreenQuad.hlsli"

Texture2D tex : register(t0);
SamplerState samLinear : register(s0);

cbuffer BlurSettings : register(b0)
{
    float2 texelSize;
    float blurStrength;
    float _padding;
}

float4 main(VSOutput input) : SV_TARGET
{
    float weights[5] = { 0.204164f, 0.304005f, 0.093913f, 0.020597f, 0.003327f };

    float4 color = tex.Sample(samLinear, input.uv) * weights[0];

    for (int i = 1; i < 5; ++i)
    {
        float offset = i * texelSize.x * blurStrength;
        color += tex.Sample(samLinear, input.uv + float2(offset, 0)) * weights[i];
        color += tex.Sample(samLinear, input.uv - float2(offset, 0)) * weights[i];
    }

    return color;
}