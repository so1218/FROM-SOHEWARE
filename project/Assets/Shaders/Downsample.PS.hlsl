#include "FullScreenQuad.hlsli"

Texture2D tex : register(t0);
SamplerState samLinear : register(s0);

cbuffer BlurSettings : register(b0)
{
    // 1ピクセルあたりのUVサイズ
    float2 texelSize;

    // ブラー強度
    float blurStrength;

    float _padding;
}

// 輝度を計算
float RGBToLuminance(float3 col)
{
    return dot(col, float3(0.2126f, 0.7152f, 0.0722f));
}

// KarisAverage用の重み計算
float KarisAverage(float3 col)
{
    float luma = RGBToLuminance(col);
    return 1.0f / (1.0f + luma);
}

float4 main(VSOutput input) : SV_TARGET
{
    float2 uv = input.uv;
    float2 t = texelSize;

    // 13点のサンプリング
    float3 a = tex.Sample(samLinear, uv + float2(-2 * t.x, 2 * t.y)).rgb;
    float3 b = tex.Sample(samLinear, uv + float2(0, 2 * t.y)).rgb;
    float3 c = tex.Sample(samLinear, uv + float2(2 * t.x, 2 * t.y)).rgb;
    float3 d = tex.Sample(samLinear, uv + float2(-t.x, t.y)).rgb;
    float3 e = tex.Sample(samLinear, uv + float2(t.x, t.y)).rgb;
    float3 f = tex.Sample(samLinear, uv + float2(-2 * t.x, 0)).rgb;
    float3 g = tex.Sample(samLinear, uv + float2(0, 0)).rgb;
    float3 h = tex.Sample(samLinear, uv + float2(2 * t.x, 0)).rgb;
    float3 i = tex.Sample(samLinear, uv + float2(-t.x, -t.y)).rgb;
    float3 j = tex.Sample(samLinear, uv + float2(t.x, -t.y)).rgb;
    float3 k = tex.Sample(samLinear, uv + float2(-2 * t.x, -2 * t.y)).rgb;
    float3 l = tex.Sample(samLinear, uv + float2(0, -2 * t.y)).rgb;
    float3 m = tex.Sample(samLinear, uv + float2(2 * t.x, -2 * t.y)).rgb;

    // 重み付け（中心付近を厚く）
    float3 result = (a + c + k + m) * 0.03125 + (b + f + h + l) * 0.0625 + (d + e + i + j) * 0.125 + g * 0.125;

    // KarisAverageを適用（Fireflies対策）
    float w = 1.0f / (1.0f + RGBToLuminance(result));
    return float4(result * w, 1.0f);
}