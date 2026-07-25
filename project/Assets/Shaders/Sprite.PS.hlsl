#include "ShaderConstants.hlsli"

ConstantBuffer<MaterialData> gMaterial : register(b0);

// 通常テクスチャ
Texture2D<float32_t4> gTexture : register(t0);
// ディソルブ用ノイズテクスチャ
Texture2D<float32_t4> gDissolveTexture : register(t1);
SamplerState gSampler : register(s0);

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

    // UV座標の変換
    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);

    // テクスチャサンプリング
    float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);

    // 色の決定 
    output.color = textureColor * gMaterial.color;
    
    // ディソルブ処理
    if (gMaterial.enableDissolve != 0)
    {
        // マテリアルのUVと同じ座標でノイズをサンプリング
        float noise = gDissolveTexture.Sample(gSampler, transformedUV.xy).r;
        
        float threshold = gMaterial.dissolveThreshold;
        
        // 閾値以下はdiscard
        if (noise <= threshold)
        {
            discard;
        }
        // エッジ発光処理
        float width = gMaterial.edgeWidth;
        float thresholdEdge = threshold + width;

        if (noise < thresholdEdge)
        {
            float t = 1.0f - ((noise - threshold) / width);
            float glowFactor = pow(t, 2.5f) * gMaterial.edgeIntensity;
            float3 edgeGlow = gMaterial.edgeColor * glowFactor;
            
            // 色を加算
            output.color.rgb += edgeGlow;
            
            // 燃え尽きる表現
            output.color.rgb = lerp(output.color.rgb, gMaterial.edgeColor * gMaterial.edgeIntensity, t);
        }
    }
    
    output.color *= gMaterial.emissiveIntensity;

    // 完全に透明なら描画しない
    if (output.color.a == 0.0)
    {
        discard;
    }

    return output;
}