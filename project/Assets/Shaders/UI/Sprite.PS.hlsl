#include "Common/ShaderConstants.hlsli"

StructuredBuffer<SpriteInstance> gInstances : register(t0);
// 通常テクスチャ
Texture2D<float4> gTextures[] : register(t0, space1);
SamplerState gSampler : register(s0);

struct SpriteVSOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    nointerpolation uint instanceId : INSTANCEID;
};

float4 main(SpriteVSOutput input) : SV_TARGET0
{
    SpriteInstance instance = gInstances[input.instanceId];

    // UV 変換 (以前の uvTransform 行列と同じ)
    float2 quadUV = input.texcoord;
    float2 transformedUV = float2(
        quadUV.x * instance.uvMatrix.x + quadUV.y * instance.uvMatrix.z + instance.uvTranslate.x,
        quadUV.x * instance.uvMatrix.y + quadUV.y * instance.uvMatrix.w + instance.uvTranslate.y);

    // テクスチャの一部 (uvRect) に割り当てる。(0,0,1,1) なら画像全体
    float2 atlasUV = instance.uvRect.xy + transformedUV * (instance.uvRect.zw - instance.uvRect.xy);

    float4 outputColor =
        gTextures[NonUniformResourceIndex(instance.textureIndex)].Sample(gSampler, atlasUV) * instance.color;

    // ディゾルブ
    if (instance.enableDissolve != 0)
    {
        float noiseValue =
            gTextures[NonUniformResourceIndex(instance.dissolveTextureIndex)].Sample(gSampler, transformedUV).r;
        float dissolveThreshold = instance.dissolveThreshold;

        if (noiseValue <= dissolveThreshold)
        {
            discard;
        }

        // 消える境界を光らせる
        float edgeThreshold = dissolveThreshold + instance.edgeWidth;
        if (noiseValue < edgeThreshold)
        {
            float edgeFactor = 1.0f - ((noiseValue - dissolveThreshold) / instance.edgeWidth);
            float glowStrength = pow(edgeFactor, 2.5f) * instance.edgeIntensity;
            outputColor.rgb += instance.edgeColor * glowStrength;
            outputColor.rgb = lerp(outputColor.rgb, instance.edgeColor * instance.edgeIntensity, edgeFactor);
        }
    }

    outputColor *= instance.emissiveIntensity;

    // 完全に透明なら描画しない
    if (outputColor.a == 0.0f)
    {
        discard;
    }
    return outputColor;
}