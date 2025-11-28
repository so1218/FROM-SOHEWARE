#include "FullScreenQuad.hlsli"

Texture2D tex : register(t0);
SamplerState samLinear : register(s0);

cbuffer BlurSettings : register(b0)
{
    float2 texelSize; // 1.0 / resolution
    float blurStrength; // ブラー範囲
    float _padding;
}

float4 main(VSOutput input) : SV_TARGET
{
    // 中心から少しずらした4点をサンプリングして平均化（ボックスフィルタ）
    float4 d = texelSize.xyxy * float4(-0.5, -0.5, 0.5, 0.5);

    float4 color = tex.Sample(samLinear, input.uv + d.xy);
    color += tex.Sample(samLinear, input.uv + d.zy);
    color += tex.Sample(samLinear, input.uv + d.xw);
    color += tex.Sample(samLinear, input.uv + d.zw);

    return color * 0.25f;
}