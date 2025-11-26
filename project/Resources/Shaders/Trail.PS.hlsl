#include "Trail.hlsli"

Texture2D<float4> gTexture : register(t0);
Texture2D<float4> gDissolveTexture : register(t1);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    // メインテクスチャと頂点カラーを掛け合わせ
    float4 texColor = gTexture.Sample(gSampler, input.texcoord);
    float4 finalColor = texColor * input.color;

    // ディゾルブ
    if (gTrailMaterial.isDissolveEnabled > 0.5)
    {
        float alpha = input.color.a;
        float noiseValue = gDissolveTexture.Sample(gSampler, input.texcoordRaw).r;

        // ノイズ値より小さい場合は描画しない
        if (alpha < noiseValue)
        {
            discard;
        }

        // 境界付近は色を光らせる
        if (alpha < noiseValue + 0.05f)
        {
            finalColor.rgb += float3(1.0, 0.5, 0.2);
        }
    }

    output.color = finalColor;

    // 完全に透明なら描画しない
    if (output.color.a <= 0.0f)
        discard;

    return output;
}