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

// ACES (Academy Color Encoding System) 近似トーンマッピング
// 映画のようなコントラストとハイライトのロールオフを実現します
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
    float4 sceneColor = gSceneTexture.Sample(gSampler, input.uv);
    float4 bloomColor = gBlurredBloom.Sample(gSampler, input.uv);

    // 1. まずシーンだけをトーンマッピングして、落ち着かせる
    float3 finalScene = sceneColor.rgb * gCombineSettings.exposure;
    finalScene = ACESFilm(finalScene);

    // 2. ブルームの色味を強調（彩度を上げる）
    // ネオン感のために、ブルームの色成分を少しブーストします
    float3 neonGlow = bloomColor.rgb * gCombineSettings.bloomIntensity;
    
    // [重要] ブルーム自体にはトーンマップをかけず、加算する！
    // これにより、強い色が白くならず、その色のまま発光します。
    float3 result = finalScene + neonGlow;

    // 必要ならここでサチュレート（1.0を超えた分はクリップされるが、色は乗った後）
    // result = saturate(result); 

    return float4(result, sceneColor.a);
}