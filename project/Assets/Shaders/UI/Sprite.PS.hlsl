#include "Common/ShaderConstants.hlsli"

StructuredBuffer<SpriteInstance> gInstances : register(t0);
// 通常テクスチャ
Texture2D<float4> gTextures[] : register(t0, space1);
SamplerState gWrapSampler : register(s0);    // UV をスクロール・繰り返しするとき
SamplerState gClampSampler : register(s1);   // 端に反対側の色がにじまないようにする

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

    // UV 変換をしていなければ Clamp で読む
    bool isUVTransformed =
        any(instance.uvMatrix != float4(1.0f, 0.0f, 0.0f, 1.0f)) || any(instance.uvTranslate != float2(0.0f, 0.0f));

    float4 textureColor;
    if (isUVTransformed)
    {
        textureColor = gTextures[NonUniformResourceIndex(instance.textureIndex)].Sample(gWrapSampler, atlasUV);
    }
    else
    {
        textureColor = gTextures[NonUniformResourceIndex(instance.textureIndex)].Sample(gClampSampler, atlasUV);
    }

    float4 outputColor = textureColor * instance.color;

    // SDF 文字。a に輪郭までの距離が入っている (0.5 が輪郭)
    if (instance.drawMode == SPRITE_DRAW_MODE_SDF_TEXT)
    {
        float signedDistance = textureColor.a;
        // 1 ピクセル分の距離の変化でぼかすと、どの大きさでもくっきりする
        float smoothing = max(fwidth(signedDistance) * 0.7f, 0.0001f);
        float fillAlpha = smoothstep(0.5f - smoothing, 0.5f + smoothing, signedDistance);

        if (instance.outlineThickness > 0.0f)
        {
            float outlineEdge = 0.5f - instance.outlineThickness;
            float outlineAlpha = smoothstep(outlineEdge - smoothing, outlineEdge + smoothing, signedDistance);
            outputColor.rgb = lerp(instance.outlineColor.rgb, instance.color.rgb, fillAlpha);
            outputColor.a = lerp(instance.outlineColor.a * outlineAlpha, instance.color.a, fillAlpha);
        }
        else
        {
            outputColor = float4(instance.color.rgb, instance.color.a * fillAlpha);
        }
    }

    // ディゾルブ
    if (instance.enableDissolve != 0)
    {
        float noiseValue =
            gTextures[NonUniformResourceIndex(instance.dissolveTextureIndex)].Sample(gWrapSampler, transformedUV).r;
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