#include "Common/Trail.hlsli"
#include "Common/MathUtils.hlsli"

Texture2D<float4> gTexture : register(t0);

SamplerState gSampler : register(s0);

struct TrailPSOutput
{
    float4 color : SV_TARGET0;
};

TrailPSOutput main(TrailVSOutput input)
{
    TrailPSOutput output;

    // メインテクスチャと頂点カラーを掛け合わせ
    float4 texColor = gTexture.Sample(gSampler, input.texcoord);
    float4 finalColor = texColor * input.color;

    output.color = finalColor;
    output.color *= gTrailMaterial.emissiveIntensity;
    
    // 完全に透明なら描画しない
    clip(finalColor.a - kEpsilon);

    return output;
}