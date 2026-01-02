#include "FullScreenQuad.hlsli"
#include "ShaderConstants.hlsli"

Texture2D gSceneTexture : register(t0); // 元のシーン
Texture2D gBloomTexture : register(t1); // Bloom用 (光のみボケ)
Texture2D gDoFTexture : register(t2); // DoF用 (全体ボケ)
Texture2D<float> gDepthTexture : register(t3); // 深度マップ

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
struct PSInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

// Tent Filter (3x3 近傍サンプリングで滑らかに拡大)
float3 UpsampleTent(Texture2D tex, SamplerState s, float2 uv, float2 texelSize, float sampleScale)
{
    float4 d = texelSize.xyxy * float4(1.0, 1.0, -1.0, 0.0) * sampleScale;

    float3 s1 = tex.Sample(s, uv - d.xy).rgb;
    float3 s2 = tex.Sample(s, uv - d.wy).rgb;
    float3 s3 = tex.Sample(s, uv - d.zy).rgb;
    float3 s4 = tex.Sample(s, uv - d.xw).rgb;
    float3 s5 = tex.Sample(s, uv).rgb;
    float3 s6 = tex.Sample(s, uv + d.xw).rgb;
    float3 s7 = tex.Sample(s, uv + d.zy).rgb;
    float3 s8 = tex.Sample(s, uv + d.wy).rgb;
    float3 s9 = tex.Sample(s, uv + d.xy).rgb;

    // 1 2 1
    // 2 4 2
    // 1 2 1  の重み配分
    return (s1 + s3 + s7 + s9) * 0.0625 + (s2 + s4 + s6 + s8) * 0.125 + s5 * 0.25;
}

// トーンマッピング
float3 ACESFilm(float3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

float4 main(VSOutput input) : SV_TARGET
{
    // 1. 各テクスチャのサンプリング
    float4 sceneColor = gSceneTexture.Sample(gSampler, input.uv);
    float4 dofColor = gDoFTexture.Sample(gSampler, input.uv); // 全体ボケ画像
    float depthVal = gDepthTexture.Sample(gSampler, input.uv);

    // Bloomテクスチャの処理 (Tent Filter)
    uint width, height;
    gBloomTexture.GetDimensions(width, height);
    float2 bloomTexelSize = float2(1.0f / float(width), 1.0f / float(height));
    float3 bloomColor = UpsampleTent(gBloomTexture, gSampler, input.uv, bloomTexelSize, 1.0f);

   // =========================================================
// 2. DoF (被写界深度) の適用 [高品質版: 光学的CoC近似]
// =========================================================

    float linearDepth = LinearizeDepth(depthVal);
   // =========================================================
    // 2. DoF (被写界深度) の適用
    // =========================================================
    
    // ★変更点: デフォルトは「ボケなし(シーンそのまま)」にする
    float3 combinedScene = sceneColor.rgb;

    // フラグが ON (0以外) の場合のみ計算する
    if (gCombineSettings.enableDoF != 0)
    {
        float focusDist = gCombineSettings.focusDistance;
        float focusRange = gCombineSettings.focusRange;

        // CoC計算
        float coc = (linearDepth - focusDist) / max(0.01f, linearDepth);
        float blurAmount = abs(coc) * (100.0f / max(0.1f, focusRange));
        float blurFactor = saturate(blurAmount);

        // 前景・背景ブレンド計算
        float mixingFactor = smoothstep(0.0f, 1.0f, blurFactor);
        
        // ボケ画像を適用
        combinedScene = lerp(sceneColor.rgb, dofColor.rgb, mixingFactor);
    }

    // 3. Bloom の合成
    // ---------------------------------------------------------
    // DoF処理後の画像に、光のあふれ(Bloom)を加算する
    float3 result = combinedScene + (bloomColor * gCombineSettings.bloomIntensity);

  // =========================================================
// 3.5 Fog (フォグ) の適用 [高品質版: 指数二乗フォグ]
// =========================================================

// フラグが ON の場合のみ計算してブレンド
    if (gCombineSettings.enableFog != 0)
    {
        float fogDensity = 0.0f;
        // ゼロ除算防止
        if (gCombineSettings.fogEnd > 0.001f)
        {
            fogDensity = 4.605f / gCombineSettings.fogEnd;
        }

        float fogDist = max(0.0f, linearDepth - gCombineSettings.fogStart);

        // 指数二乗フォグ計算 (重いexp/pow計算をスキップできるメリットがある)
        float fogFactor = exp(-pow(fogDist * fogDensity, 2.0f));
        fogFactor = saturate(1.0f - fogFactor);

        // フォグ色をブレンド
        result = lerp(result, gCombineSettings.fogColor, fogFactor);
    }
    
    // 4. トーンマッピング & 出力調整
    if (any(isnan(result)))
    {
        result = float3(0.0, 0.0, 0.0);
    }

    result = clamp(result, 0.0, 65504.0);
    result = ACESFilm(result);

    return float4(result, 1.0f);
}