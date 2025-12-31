#include "FullScreenQuad.hlsli"

Texture2D tex : register(t0);
SamplerState samLinear : register(s0);

cbuffer BlurSettings : register(b0)
{
    float2 texelSize; // 1.0/resolution
    float blurStrength; // ブラー範囲
    float _padding;
}
float4 main(VSOutput input) : SV_TARGET
{
    // Karis Average (チラつき防止の高品質ダウンサンプル)
    float2 uv = input.uv;

    // オフセット計算 (テクセルサイズ分ずらす)
    float4 d = texelSize.xyxy * float4(-1.0, -1.0, 1.0, 1.0);

    float3 s1 = tex.Sample(samLinear, uv + d.xy).rgb; // 左上
    float3 s2 = tex.Sample(samLinear, uv + d.zy).rgb; // 右上
    float3 s3 = tex.Sample(samLinear, uv + d.xw).rgb; // 左下
    float3 s4 = tex.Sample(samLinear, uv + d.zw).rgb; // 右下
    float3 s5 = tex.Sample(samLinear, uv).rgb; // 中心

    // 重み付け平均
    // 中心を強く、周囲を弱く、光の芯を残しつつ滑らかにする処理
    float3 result = (s1 + s2 + s3 + s4) * 0.125f + s5 * 0.5f;

    return float4(result, 1.0f);
}