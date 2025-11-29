#include "FullScreenQuad.hlsli"
#include "ShaderConstants.hlsli"

Texture2D gSceneTexture : register(t0); // 元のシーン
Texture2D gBlurredBloom : register(t1); // ブラー済みBloomテクスチャ
SamplerState gSampler : register(s0);

ConstantBuffer<CombineSettings> gCombineSettings : register(b0);
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

// 簡易トーンマッピング (Reinhard)
// これにより、強烈な光(10.0など)を 0.0~1.0 のモニタ表示範囲に自然に収める
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
    // 1. シーン情報の取得
    float4 sceneColor = gSceneTexture.Sample(gSampler, input.uv);
    
    // 2. Bloomテクスチャのテクセルサイズを自動取得
    // (C++で定数バッファを作らなくても、HLSL側で画像の大きさを調べられます)
    uint width, height;
    gBlurredBloom.GetDimensions(width, height);
    float2 bloomTexelSize = float2(1.0f / float(width), 1.0f / float(height));

    // 3. Tent Filterを使って滑らかに拡大サンプリング
    // 半径(sampleScale)を少し広め(1.0~2.0)にとるとよりフワッとなります
    float3 bloomColor = UpsampleTent(gBlurredBloom, gSampler, input.uv, bloomTexelSize, 1.0f);

    float3 result = float3(0, 0, 0);

    // 4. 合成処理 (基本は EffectMode 1 の加算)
    if (gCombineSettings.effectMode == 0) // Halo (輝度ベース)
    {
        float luminance = dot(bloomColor.rgb, float3(0.299, 0.587, 0.114));
        float3 haloColor = float3(1.0, 1.0, 1.0) * luminance * 1.5;
        haloColor = haloColor * gCombineSettings.bloomIntensity;
        result = sceneColor.rgb + haloColor;
    }
    else if (gCombineSettings.effectMode == 2) // Overlay
    {
        float3 overlay = 1.0 - (1.0 - bloomColor.rgb) * (1.0 - sceneColor.rgb);
        result = lerp(sceneColor.rgb, overlay, gCombineSettings.bloomIntensity);
    }
    else // Default: Standard Additive (Neon) ★ここがパーティクル用におすすめ
    {
        // 単純加算。BloomColor自体がHDR(1.0以上)を持っている場合、強烈に発光する
        result = sceneColor.rgb + bloomColor * gCombineSettings.bloomIntensity;
    }
    
    if (any(isnan(result)))
    {
        result = float3(0.0, 0.0, 0.0);
    }
    
    result = clamp(result, 0.0, 65504.0);

    // 5. トーンマッピング
    // saturate() の代わりに ToneMap() を使うことで、
    // 1.0を超えた光の「芯」が白く飛び、周囲が色づく「発光感」が出る
    result = ACESFilm(result);

    return float4(result, sceneColor.a);
}