#include "Common/ShaderConstants.hlsli"
#include "Common/FullScreenQuad.hlsli"

Texture2D gTexture : register(t0);
SamplerState samLinear : register(s0);
ConstantBuffer<BloomSettings> gBloomSettings : register(b0);

float4 main(VSOutput input) : SV_TARGET
{
    float2 uv = input.uv;
    float2 texelSize = gBloomSettings.texelSize; // 縮小前のテクスチャの 1.0/Width, 1.0/Height

    // 13タップフィルタ（ホタル現象やチラつきを抑えつつ滑らかにする）
    float4 A = gTexture.Sample(samLinear, uv + float2(-2.0, -2.0) * texelSize);
    float4 B = gTexture.Sample(samLinear, uv + float2(0.0, -2.0) * texelSize);
    float4 C = gTexture.Sample(samLinear, uv + float2(2.0, -2.0) * texelSize);
    float4 D = gTexture.Sample(samLinear, uv + float2(-2.0, 0.0) * texelSize);
    float4 E = gTexture.Sample(samLinear, uv + float2(0.0, 0.0) * texelSize);
    float4 F = gTexture.Sample(samLinear, uv + float2(2.0, 0.0) * texelSize);
    float4 G = gTexture.Sample(samLinear, uv + float2(-2.0, 2.0) * texelSize);
    float4 H = gTexture.Sample(samLinear, uv + float2(0.0, 2.0) * texelSize);
    float4 I = gTexture.Sample(samLinear, uv + float2(2.0, 2.0) * texelSize);
    float4 J = gTexture.Sample(samLinear, uv + float2(-1.0, -1.0) * texelSize);
    float4 K = gTexture.Sample(samLinear, uv + float2(1.0, -1.0) * texelSize);
    float4 L = gTexture.Sample(samLinear, uv + float2(-1.0, 1.0) * texelSize);
    float4 M = gTexture.Sample(samLinear, uv + float2(1.0, 1.0) * texelSize);

    float4 color = E * 0.125;
    color += (A + C + G + I) * 0.03125;
    color += (B + D + F + H) * 0.0625;
    color += (J + K + L + M) * 0.125;

    return color;
}