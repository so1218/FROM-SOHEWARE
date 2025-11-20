#include "Trail.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    // テクスチャをサンプリング
    float4 textureColor = gTexture.Sample(gSampler, input.texcoord);

    // テクスチャ色と頂点カラーを乗算
    output.color = textureColor * input.color;

    // 透明なら描画しない
    if (output.color.a <= 0.0f)
        discard;

    return output;
}