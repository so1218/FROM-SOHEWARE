#include "Common/FullScreenQuad.hlsli"

Texture2D gTexture : register(t0);
SamplerState samLinear : register(s0);

float4 main(VSOutput input) : SV_TARGET
{
    // 入力されたテクスチャを返す
    return gTexture.Sample(samLinear, input.uv);
}