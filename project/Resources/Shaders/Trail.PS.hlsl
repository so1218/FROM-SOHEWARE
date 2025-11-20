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

    // 1. メインテクスチャ
    float4 texColor = gTexture.Sample(gSampler, input.texcoord);
    float4 finalColor = texColor * input.color;

   // -------------------------------------------------
    // ★ディゾルブ (自然な消滅)
    // -------------------------------------------------
    // フラグが立っているときだけ計算する
    if (gTrailMaterial.isDissolveEnabled > 0.5)
    {
        float alpha = input.color.a;
        float noiseValue = gDissolveTexture.Sample(gSampler, input.texcoordRaw).r;

        if (alpha < noiseValue)
        {
            discard;
        }
        
        if (alpha < noiseValue + 0.05f)
        {
            finalColor.rgb += float3(1.0, 0.5, 0.2);
        }
    }

    output.color = finalColor;

    if (output.color.a <= 0.0f)
        discard;

    return output;
}