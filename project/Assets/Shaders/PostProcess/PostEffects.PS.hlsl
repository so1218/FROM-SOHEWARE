#include "Common/FullScreenQuad.hlsli"
#include "Common/ShaderConstants.hlsli"
#include "Common/MathUtils.hlsli"

Texture2D gTexture : register(t0);
Texture2D gLutTexture : register(t1);
SamplerState gSampler : register(s0);
SamplerState gClampSampler : register(s1);

ConstantBuffer<PostEffectData> gData : register(b0);

// ピクセル化
float2 ApplyPixelation(float2 uv)
{
    float2 pixelSizeUV = gData.pixelationSize / gData.screenResolution;
    return floor(uv / pixelSizeUV) * pixelSizeUV;
}

// 色かぶり
float3 ApplyColorTint(float3 color)
{
    float3 mulTint = lerp(color, color * gData.tintColor, gData.tintMulColorAmount);
    float3 addTint = saturate(color + gData.tintColor * gData.tintAddColorAmount);
    float3 screenTint = 1.0 - (1.0 - color) * (1.0 - gData.tintColor * gData.tintScreenColorAmount);
    return (mulTint + addTint + screenTint) / 3.0;
}

// ビネット
float3 ApplyVignette(float3 color, float2 uv)
{
    float2 center = uv - 0.5f;
    
    // 0除算ガード
    float2 scale = max(gData.vignetteEllipseScale, float2(0.001f, 0.001f));
    float dist = length(center / scale);

    float fadeLength = max(gData.vignetteSoftness, kEpsilon);
    float darkness = (dist - gData.vignetteRadius) / fadeLength;

    darkness = smoothstep(0.0f, 1.0f, darkness);
    darkness *= gData.vignetteAmount;

    // ディザリング
    float dither = (InterleavedGradientNoise(uv * gData.screenResolution.xy) - 0.5f) / 255.0f;
    darkness += dither * 2.0f;
    
    float blendAmount = saturate(darkness);

    // 元の色から vignetteColor へ補間
    return lerp(color, gData.vignetteColor.rgb, blendAmount);
}

// 色収差
float4 SampleChromaticAberration(float2 uv)
{
    // UV上のオフセット量を計算
    float2 offset = gData.chromaOffset;

    // offset でずらす
    float r = gTexture.Sample(gSampler, saturate(uv + offset)).r;
    float g = gTexture.Sample(gSampler, uv).g;
    float b = gTexture.Sample(gSampler, saturate(uv - offset)).b;

    return float4(r, g, b, 1.0f);
}

// ラディアルブラー
float4 ApplyRadialBlur(float2 uv)
{
    const int SAMPLES = 12;
    float4 color = float4(0, 0, 0, 0);
    
    // 中心からの方向ベクトル
    float2 dir = uv - gData.radialBlurCenter;
    
    // 中心に向かって放射状にサンプリング位置をずらす
    for (int i = 0; i < SAMPLES; i++)
    {
        float t = float(i) / float(SAMPLES - 1);
        
        // 強度に応じて UV をオフセット
        float2 sampleUV = uv - dir * (gData.radialBlurStrength * t);
        
        color += gTexture.Sample(gSampler, sampleUV);
    }
    
    return color / float(SAMPLES);
}

// LUTを使ったカラーグレーディング
float3 ApplyColorGradingLUT(float3 color)
{
    // LUTのサイズ
    const float LUT_SIZE = 32.0f;
    const float MAX_COLOR = LUT_SIZE - 1.0f;

    // 入力カラーをLUTのインデックスに変換 (0.0～1.0 -> 0.0～31.0)
    float3 lutIndex = saturate(color) * MAX_COLOR;

    // 2D展開されたLUT（1024x32）からサンプリングするためのUV計算
    float sliceX = floor(lutIndex.z); // 現在のスライス
    float nextSliceX = min(sliceX + 1.0f, MAX_COLOR); // 次のスライス（補間用）

    // ピクセル中心に合わせるためのハーフピクセルオフセット
    float halfPixelX = 0.5f / (LUT_SIZE * LUT_SIZE);
    float halfPixelY = 0.5f / LUT_SIZE;

    // 現在のスライスのUV
    float2 uv1;
    uv1.x = (sliceX * LUT_SIZE + lutIndex.x + 0.5f) / (LUT_SIZE * LUT_SIZE);
    uv1.y = (lutIndex.y + 0.5f) / LUT_SIZE;

    // 次のスライスのUV
    float2 uv2;
    uv2.x = (nextSliceX * LUT_SIZE + lutIndex.x + 0.5f) / (LUT_SIZE * LUT_SIZE);
    uv2.y = (lutIndex.y + 0.5f) / LUT_SIZE;

    // 2つのスライスからサンプリング
    float3 color1 = gLutTexture.SampleLevel(gClampSampler, uv1, 0).rgb;
    float3 color2 = gLutTexture.SampleLevel(gClampSampler, uv2, 0).rgb;

    // Z軸（青成分）の端数で線形補間
    float zFraction = frac(lutIndex.z);
    return lerp(color1, color2, zFraction);
}

float4 main(VSOutput input) : SV_TARGET
{
    float2 uv = input.uv;
    float4 finalColor = float4(0, 0, 0, 1);

    // UV を更新
    if (gData.flag & PIXELATION)
    {
        uv = ApplyPixelation(uv);
    }

    // 変形された UV を使ってテクスチャをサンプリング
    if (gData.flag & CHROM_ABERRATION)
    {
        finalColor = SampleChromaticAberration(uv);
    }
    else if (gData.flag & RADIAL_BLUR)
    {
        finalColor = ApplyRadialBlur(uv);
    }
    else
    {
        // 標準サンプリング
        finalColor = gTexture.Sample(gSampler, uv);
    }
    
    // 色補正・フィルタ処理
    if (gData.flag & COLOR_TINT)
        finalColor.rgb = ApplyColorTint(finalColor.rgb);
    if (gData.flag & VIGNETTE)
        finalColor.rgb = ApplyVignette(finalColor.rgb, uv);

    if (gData.flag & SCREEN_NOISE)
    {
        float2 scaledUV = uv * gData.screenResolution.xy * gData.noiseScale;
        float2 timeOffset = float2(gData.totalTime * gData.noiseSpeed, gData.totalTime * gData.noiseSpeed * 1.7f);

        float3 noiseColor = Hash33(float3(scaledUV + timeOffset, gData.totalTime)) * 0.5f;
        finalColor.rgb += noiseColor * gData.noiseAmount;
    }

    // LUT
    if (gData.flag & COLOR_GRADING_LUT)
    {
        finalColor.rgb = ApplyColorGradingLUT(finalColor.rgb);
    }

    return finalColor;
}