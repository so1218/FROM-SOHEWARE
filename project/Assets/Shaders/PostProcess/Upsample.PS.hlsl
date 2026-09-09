#include "Common/ShaderConstants.hlsli"
#include "Common/FullScreenQuad.hlsli"

Texture2D gTexture : register(t0); // 1つ下の低解像度テクスチャ
Texture2D gCurrentTexture : register(t1); // 現在の解像度のテクスチャ（合成用）
SamplerState samLinear : register(s0);
ConstantBuffer<BloomSettings> gBloomSettings : register(b0);

float4 main(VSOutput input) : SV_TARGET
{
    float2 uv = input.uv;
    // radius で光の広がり具合を調整（通常は 1.0 ～ 2.0 程度）
    float2 offset = gBloomSettings.texelSize * gBloomSettings.radius;

    float x = offset.x;
    float y = offset.y;

    // 9タップ・テントフィルタ
    float4 a = gTexture.Sample(samLinear, uv + float2(-x, -y));
    float4 b = gTexture.Sample(samLinear, uv + float2(0, -y));
    float4 c = gTexture.Sample(samLinear, uv + float2(x, -y));
    float4 d = gTexture.Sample(samLinear, uv + float2(-x, 0));
    float4 e = gTexture.Sample(samLinear, uv + float2(0, 0));
    float4 f = gTexture.Sample(samLinear, uv + float2(x, 0));
    float4 g = gTexture.Sample(samLinear, uv + float2(-x, y));
    float4 h = gTexture.Sample(samLinear, uv + float2(0, y));
    float4 i = gTexture.Sample(samLinear, uv + float2(x, y));

    float4 upsampleColor = e * 4.0;
    upsampleColor += (b + d + f + h) * 2.0;
    upsampleColor += (a + c + g + i) * 1.0;
    upsampleColor *= (1.0 / 16.0);

    return upsampleColor;
}