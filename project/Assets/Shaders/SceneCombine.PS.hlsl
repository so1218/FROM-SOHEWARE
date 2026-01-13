#include "FullScreenQuad.hlsli"
#include "ShaderConstants.hlsli"

Texture2D gSceneTexture : register(t0); // 3Dレンダリング結果
Texture2D gDoFTexture : register(t1); // DoF用ボケ画像
Texture2D<float> gDepthTexture : register(t2); // 深度マップ
Texture2D gGodRayTexture : register(t3); // ゴッドレイ画像

SamplerState gSampler : register(s0);

ConstantBuffer<CombineSettings> gCombineSettings : register(b0);
ConstantBuffer<FrameData> gFrameData : register(b1);

// 深度リニア化関数
float LinearizeDepth(float d)
{
    float n = gFrameData.nearClip;
    float f = gFrameData.farClip;
    return (n * f) / (f - d * (f - n));
}

float4 main(VSOutput input) : SV_TARGET
{
    float4 sceneColor = gSceneTexture.Sample(gSampler, input.uv);
    float4 dofColor = gDoFTexture.Sample(gSampler, input.uv);
    float depthVal = gDepthTexture.Sample(gSampler, input.uv);
    float3 godRayColor = gGodRayTexture.Sample(gSampler, input.uv).rgb;

    float linearDepth = LinearizeDepth(depthVal);
    float3 result = sceneColor.rgb;

    // DoF
    if (gCombineSettings.enableDoF != 0)
    {
        float focusDist = gCombineSettings.focusDistance;
        float focusRange = gCombineSettings.focusRange;
        float coc = (linearDepth - focusDist) / max(0.01f, linearDepth);
        float blurAmount = abs(coc) * (100.0f / max(0.1f, focusRange));
        float mixingFactor = smoothstep(0.0f, 1.0f, saturate(blurAmount));

        result = lerp(sceneColor.rgb, dofColor.rgb, mixingFactor);
    }

    // GodRay加算
    result += (godRayColor * gCombineSettings.godRayIntensity);

    // フォグ
    if (gCombineSettings.enableFog != 0)
    {
        float fogDensity = (gCombineSettings.fogEnd > 0.001f) ? (4.605f / gCombineSettings.fogEnd) : 0.0f;
        float fogDist = max(0.0f, linearDepth - gCombineSettings.fogStart);
        float fogFactor = saturate(1.0f - exp(-pow(fogDist * fogDensity, 2.0f)));
        
        result = lerp(result, gCombineSettings.fogColor, fogFactor);
    }

    // NaN対策
    if (any(isnan(result)))
        result = float3(0.0, 0.0, 0.0);
    
    return float4(result, 1.0f);
}