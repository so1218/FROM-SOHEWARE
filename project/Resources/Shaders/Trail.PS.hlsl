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

    // テクスチャからサンプリング
    float4 textureColor = gTexture.Sample(gSampler, input.texcoord);
    
    // テクスチャの色 × 頂点カラー（フェード値含む）
    output.color = textureColor * input.color;

    // 必要であればアルファテスト（完全に透明なら描画しない）
    if (output.color.a <= 0.0f)
    {
        discard;
    }

    return output;
}