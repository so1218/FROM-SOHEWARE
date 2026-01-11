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

// Karis Average用の重み計算
// 高輝度ピクセルの影響を抑えてチラつきを防ぐ
float KarisAverage(float3 col)
{
    float luma = RGBToLuminance(col);
    return 1.0f / (1.0f + luma);
}

float4 main(VSOutput input) : SV_TARGET
{
    float2 uv = input.uv;

    // 周辺4点と中心をサンプリング
    float4 d =
        texelSize.xyxy * float4(-1.0, -1.0, 1.0, 1.0);

    float3 s1 = tex.Sample(samLinear, uv + d.xy).rgb;
    float3 s2 = tex.Sample(samLinear, uv + d.zy).rgb;
    float3 s3 = tex.Sample(samLinear, uv + d.xw).rgb;
    float3 s4 = tex.Sample(samLinear, uv + d.zw).rgb;
    float3 s5 = tex.Sample(samLinear, uv).rgb;

    // 各サンプルの重みを計算
    float w1 = KarisAverage(s1);
    float w2 = KarisAverage(s2);
    float w3 = KarisAverage(s3);
    float w4 = KarisAverage(s4);
    float w5 = KarisAverage(s5);

    // 重み付きで合成
    float3 result =
        (s1 * w1) +
        (s2 * w2) +
        (s3 * w3) +
        (s4 * w4) +
        (s5 * w5);

    // 正規化
    float totalWeight = w1 + w2 + w3 + w4 + w5;
    result /= max(totalWeight, 0.0001f);

    return float4(result, 1.0f);
}