#include "ShaderConstants.hlsli"

ConstantBuffer<LeafMaterialData> gMaterial : register(b2);
Texture2D<float4> gAlbedoAlphaTex : register(t8);
SamplerState gAnisoSampler : register(s3);

struct ShadowVSOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

void TreeFoliageShadowPS(ShadowVSOutput input)
{
    float alpha = gAlbedoAlphaTex.Sample(gAnisoSampler, input.texcoord).a;
    
    // アルファカットオフ以下のピクセルは影を落とさない
    clip(alpha - gMaterial.alphaCutoff);
}