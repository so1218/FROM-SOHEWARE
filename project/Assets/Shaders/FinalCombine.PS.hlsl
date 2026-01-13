#include "FullScreenQuad.hlsli"
#include "ShaderConstants.hlsli"

// 前半の結果 + UIが描かれたテクスチャ
Texture2D gCompositeTexture : register(t0);
// Bloomのボケ画像
Texture2D gBloomTexture : register(t1);

SamplerState gSampler : register(s0);

ConstantBuffer<CombineSettings> gCombineSettings : register(b0);

// TentFilter
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
    // これまでの描画結果を取得
    float3 sceneColor = gCompositeTexture.Sample(gSampler, input.uv).rgb;

    // Bloomテクスチャをアップサンプル
    uint width, height;
    gBloomTexture.GetDimensions(width, height);
    float2 bloomTexelSize = float2(1.0f / float(width), 1.0f / float(height));
    float3 bloomColor = UpsampleTent(gBloomTexture, gSampler, input.uv, bloomTexelSize, 1.0f);
    
    // Bloom合成
    float3 result = sceneColor + (bloomColor * gCombineSettings.bloomIntensity);

    // 最後にまとめてトーンマップ
    // HDR(0〜10000) -> SDR(0〜1)への変換
    result = ACESFilm(result);

    return float4(result, 1.0f);
}