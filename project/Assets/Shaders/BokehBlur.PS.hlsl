#include "FullScreenQuad.hlsli"
#include "ShaderConstants.hlsli"

// シーンカラー
Texture2D gSceneTexture : register(t0);
// 深度
Texture2D<float> gDepthTexture : register(t1);
SamplerState gSampler : register(s0);

// DoF設定
ConstantBuffer<DoFSettings> gDoFSettings : register(b0);
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
    // ピント中心からの距離
    float diff = depth - gDoFSettings.focusDistance;
    
    // ピントが完全に合う範囲
    float deadZone = gDoFSettings.focusRange * 0.5f;
    
    // デッドゾーンからはみ出た距離だけを取得
    float outOfFocusDist = max(0.0f, abs(diff) - deadZone) * sign(diff);
    
    // はみ出た距離を transitionRange で割って、徐々にボケさせる
    float coc = outOfFocusDist / max(0.001f, gDoFSettings.transitionRange);
    
    return clamp(coc, -1.0f, 1.0f);
}

// 黄金角スパイラルサンプリング
static const float GOLDEN_ANGLE = 2.39996323f;
static const int SAMPLE_COUNT = 64;

float4 main(VSOutput input) : SV_TARGET
{
    float2 uv = input.uv;

    float centerDepth = LinearizeDepth(gDepthTexture.SampleLevel(gSampler, uv, 0));
    
    float centerCoC = GetSignedCoC(centerDepth);
    float centerAbsCoC = abs(centerCoC);
    
    // ピント範囲内、またはUIでボケ半径を0にしている時は、即座に元絵を返す
    if (centerAbsCoC < 0.001f || gDoFSettings.bokehRadius <= 0.0f)
    {
        return float4(0.0f, 0.0f, 0.0f, 0.0f);
    }
    
    float3 finalColor = 0;
    float totalWeight = 0;
    
    // ボケ量(CoC)に応じてサンプリングする円の大きさを変える
    float currentRadius = centerAbsCoC * gDoFSettings.bokehRadius;
    float aspect = (gFrameData.screenResolution.x / 2.0f) / (gFrameData.screenResolution.y / 2.0f);
    
    // UV座標から疑似乱数（回転角度）を作る
    float randomNoise = frac(sin(dot(uv, float2(12.9898f, 78.233f))) * 43758.5453f);
    float randomAngle = randomNoise * 3.14159265f * 2.0f; // 0 ～ 2πのランダムな角度

    // ボケサンプリング
    for (int i = 0; i < SAMPLE_COUNT; i++)
    {
        // i * GOLDEN_ANGLE に、ピクセル固有のランダム角度を足す
        float theta = i * GOLDEN_ANGLE + randomAngle;
        float r = sqrt((float) i / SAMPLE_COUNT);

        float2 offset = float2(cos(theta), sin(theta)) * r * currentRadius;
        offset.x /= aspect;
        offset /= gFrameData.screenResolution.xy / 2.0f;

        float2 sampleUV = saturate(uv + offset);

        // ループ内も SampleLevel を使用
        float3 sampleColor = gSceneTexture.SampleLevel(gSampler, sampleUV, 0).rgb;

        // フチを少し滑らかにして円盤にする
        float weight = smoothstep(1.0f, 0.8f, r);

        // 玉ボケ強調処理
        float luminance = dot(sampleColor, float3(0.299, 0.587, 0.114));
        float highlight = max(0.0f, luminance - gDoFSettings.bokehHighlightThreshold);
        weight *= 1.0f + pow(highlight, 2.0f) * gDoFSettings.bokehHighlightIntensity;

        finalColor += sampleColor * weight;
        totalWeight += weight;
    }

    float3 bokehResult = finalColor / max(0.0001f, totalWeight);

    // ピントが合っている所から外れる境界を、ほんの少しだけブレンドする
    float mixing = smoothstep(0.0f, 0.2f, centerAbsCoC);

    return float4(bokehResult, mixing);
}
