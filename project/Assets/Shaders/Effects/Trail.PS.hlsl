#include "Common/Trail.hlsli"
#include "Common/MathUtils.hlsli"

Texture2D<float4> gTexture : register(t0);
Texture2D<float4> gDissolveTexture : register(t1);

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
    
    // 雷専用の見た目処理
    if (gTrailMaterial.jitterMode == 2)
    {
        // 雷のチカチカ演出
        float flickerTime = gFrameData.gTime * 60.0f + gTrailMaterial.instanceSeed;
        float flicker = Hash11(flickerTime);
        float flash = (flicker > 0.3f) ? 1.0f : 0.2f;
        
        finalColor.rgb *= flash;
    }

    // ディゾルブ
    if (gTrailMaterial.isDissolveEnabled > 0.5f)
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
            finalColor.rgb += float3(1.0f, 0.5f, 0.2f);
        }
    }

    output.color = finalColor;
    output.color *= gTrailMaterial.emissiveIntensity;
    
    // 完全に透明なら描画しない
    if (output.color.a <= 0.0f)
        discard;

    return output;
}