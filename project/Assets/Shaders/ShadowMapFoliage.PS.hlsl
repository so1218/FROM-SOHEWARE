#include "ShaderConstants.hlsli"

ConstantBuffer<LeafMaterialData> gMaterial : register(b5);
SamplerState gAnisoSampler : register(s3);
Texture2D<float4> gAlbedoAlphaTex : register(t12);

struct LeafShadowVSOutput
{
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD0;
};

void LeafShadowPS(LeafShadowVSOutput input)
{
    // アルファ値をサンプルして、透明部分の影を削る（これでシルエットが出る）
    float alpha = gAlbedoAlphaTex.Sample(gAnisoSampler, input.texcoord).a;
    
    // アルファカットオフ以下のピクセルは影を描画しない（破棄する）
    clip(alpha - gMaterial.alphaCutoff);
    
    // 色出力やライティング計算は一切不要（深度バッファのみ書き込まれる）
}