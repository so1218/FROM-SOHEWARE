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
    // シーン情報の取得
    float4 sceneColor = gSceneTexture.Sample(gSampler, input.uv);
    
    // Bloomテクスチャのテクセルサイズを自動取得
    uint width, height;
    gBlurredBloom.GetDimensions(width, height);
    float2 bloomTexelSize = float2(1.0f / float(width), 1.0f / float(height));

    // Tent Filterを使って滑らかに拡大サンプリング
    float3 bloomColor = UpsampleTent(gBlurredBloom, gSampler, input.uv, bloomTexelSize, 1.0f);

    // 合成処理
    float3 result = sceneColor.rgb + (bloomColor * gCombineSettings.bloomIntensity);
    
    // 計算エラーで画面が真っ黒になるのを防ぐ
    if (any(isnan(result)))
    {
        result = float3(0.0, 0.0, 0.0);
    }
    
    // HDR値のクランプ (無限大の発散を防ぐ)
    result = clamp(result, 0.0, 65504.0);

    // トーンマッピング
    // Bloomを加算して輝度が高くなった状態から、モニタ表示用の0.0-1.0に落とし込む
    result = ACESFilm(result);

    return float4(result, sceneColor.a);
}