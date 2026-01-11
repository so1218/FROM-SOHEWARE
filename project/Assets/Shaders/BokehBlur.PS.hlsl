#include "FullScreenQuad.hlsli"
#include "ShaderConstants.hlsli"

// シーンカラー
Texture2D gSceneTexture : register(t0);
// 深度
Texture2D<float> gDepthTexture : register(t1);
SamplerState gSampler : register(s0);

// DoF設定
ConstantBuffer<DoFSettings> gDoFSettings : register(b0);
// フレーム共通データ
ConstantBuffer<FrameData> gFrameData : register(b1);

// 深度をリニア化
float LinearizeDepth(float d)
{
    float n = gFrameData.nearClip;
    float f = gFrameData.farClip;
    return (n * f) / (f - d * (f - n));
}

// 符号付きCoC（-：手前ボケ, +：奥ボケ）
float GetSignedCoC(float depth)
{
    float coc = (depth - gDoFSettings.focusDistance) / max(0.0001f, depth);
    float factor = 100.0f / max(0.1f, gDoFSettings.focusRange);
    return clamp(coc * factor, -1.0f, 1.0f);
}

// 黄金角スパイラルサンプリング
static const float GOLDEN_ANGLE = 2.39996323f;
static const int SAMPLE_COUNT = 64;

float4 main(VSOutput input) : SV_TARGET
{
    float2 uv = input.uv;

    // 中心ピクセル
    float centerDepth = LinearizeDepth(gDepthTexture.Sample(gSampler, uv));
    float centerCoC = GetSignedCoC(centerDepth);
    float centerAbsCoC = abs(centerCoC);

    float3 finalColor = 0;
    float totalWeight = 0;

    // アスペクト補正
    float aspect = (gFrameData.screenResolution.x / 2.0f) / (gFrameData.screenResolution.y / 2.0f);

    // ボケサンプリング
    for (int i = 0; i < SAMPLE_COUNT; i++)
    {
        // スパイラル配置
        float theta = i * GOLDEN_ANGLE;
        float r = sqrt((float) i / SAMPLE_COUNT);

        float2 offset = float2(cos(theta), sin(theta)) * r * gDoFSettings.bokehRadius;
        offset.x /= aspect;
        offset /= gFrameData.screenResolution.xy / 2.0f;

        float2 sampleUV = uv + offset;

        // サンプル
        float3 sampleColor = gSceneTexture.SampleLevel(gSampler, sampleUV, 0).rgb;
        float sampleDepth = LinearizeDepth(gDepthTexture.SampleLevel(gSampler, sampleUV, 0));
        float sampleCoC = GetSignedCoC(sampleDepth);
        float sampleAbsCoC = abs(sampleCoC);

        // ウェイト計算（Foreground Bleeding対応）
        float weight;
        if (sampleDepth > centerDepth)
        {
            // 奥ボケ
        }
        else
        {
            // 手前ボケ
            weight = saturate(sampleAbsCoC * 2.0f);
        }

        // ハイライト強調（玉ボケ）
        float luminance = dot(sampleColor, float3(0.299, 0.587, 0.114));
        weight *= smoothstep(0.7f, 1.0f, luminance) * 4.0f + 1.0f;

        finalColor += sampleColor * weight;
        totalWeight += weight;
    }

    // ウェイトなしなら元の色
    if (totalWeight < 0.001f)
    {
        return gSceneTexture.Sample(gSampler, uv);
    }

    return float4(finalColor / totalWeight, 1.0f);
}
