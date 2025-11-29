#include "ShaderConstants.hlsli"

ConstantBuffer<MaterialData> gMaterial : register(b0);

// テクスチャとサンプラー (RootParameter[2] -> t0, StaticSampler[0] -> s0)
Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

// VSからの入力
struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

struct PixelShaderInput
{
    float32_t4 position : SV_POSITION;
    float32_t2 texcoord : TEXCOORD0;
};

PixelShaderOutput main(PixelShaderInput input)
{
    PixelShaderOutput output;

    // 1. UV座標の変換 (スプライトの切り抜きや回転などに必要)
    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);

    // 2. テクスチャサンプリング
    float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);

    // 3. 色の決定 (テクスチャ色 × マテリアル色)
    // ライティング計算は行わない
    output.color = textureColor * gMaterial.color;
    
    output.color *= gMaterial.emissiveIntensity;

    // 4. アルファテスト (完全に透明なら描画しない)
    if (output.color.a == 0.0)
    {
        discard;
    }

    return output;
}