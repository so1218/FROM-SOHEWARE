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

    // 2. DoF (被写界深度) の適用
    // ---------------------------------------------------------
    // 深度をリニア(メートル単位)に変換
    float linearDepth = LinearizeDepth(depthVal);
    
    // ピント位置との差分を計算
    float distToFocus = abs(linearDepth - gCombineSettings.focusDistance);
    
    // 差分に基づいてボケ具合(0.0～1.0)を決定
    // smoothstep(min, max, x) で滑らかに補間
    float blurFactor = smoothstep(0.0f, gCombineSettings.focusRange, distToFocus);

    // クッキリ画像(sceneColor) と ボケ画像(dofColor) を混ぜる
    float3 combinedScene = lerp(sceneColor.rgb, dofColor.rgb, blurFactor);


    // 3. Bloom の合成
    // ---------------------------------------------------------
    // DoF処理後の画像に、光のあふれ(Bloom)を加算する
    float3 result = combinedScene + (bloomColor * gCombineSettings.bloomIntensity);

    
    // 4. トーンマッピング & 出力調整
    // ---------------------------------------------------------
    if (any(isnan(result)))
    {
        result = float3(0.0, 0.0, 0.0);
    }

    result = clamp(result, 0.0, 65504.0);
    result = ACESFilm(result);

    return float4(result, 1.0f);
}