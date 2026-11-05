#include "Common/ShaderConstants.hlsli"
#include "Common/FullScreenQuad.hlsli"
#include "Common/CameraUtils.hlsli"
#include "Common/MathUtils.hlsli"

Texture2D<float4> gSceneTexture : register(t0);
Texture2D<float> gDepthTexture : register(t1);
SamplerState gSampler : register(s0);

ConstantBuffer<DoFSettings> gDoFSettings : register(b0);
ConstantBuffer<FrameData> gFrameData : register(b1);

// サンプルを黄金角ずつ回して、円の中に偏りなく並べる
static const float kGoldenAngle = 2.39996323f;
static const int kSampleCount = 64;

// ぼけの大きさ。-1~1 で、負が手前、正が奥。focusRange の中はぼけない
float CalculateSignedCoC(float viewDepth)
{
    float distanceFromFocus = viewDepth - gDoFSettings.focusDistance;
    float outOfFocusDistance = max(abs(distanceFromFocus) - gDoFSettings.focusRange * 0.5f, 0.0f);
    return clamp(sign(distanceFromFocus) * outOfFocusDistance / max(gDoFSettings.transitionRange, kEpsilon), -1.0f, 1.0f);
}

// rgb はぼかした色、a は元の絵と混ぜる割合。ピントが合っているところは a が 0
float4 main(VSOutput input) : SV_TARGET
{
    uint depthWidth, depthHeight;
    gDepthTexture.GetDimensions(depthWidth, depthHeight);
    int2 depthCoord = min(int2(input.uv * float2(depthWidth, depthHeight)), int2(depthWidth, depthHeight) - 1);
    float viewDepth = LinearizeDepth(gDepthTexture.Load(int3(depthCoord, 0)), gFrameData.nearClip, gFrameData.farClip);

    float cocSize = abs(CalculateSignedCoC(viewDepth));
    if (cocSize < 0.001f || gDoFSettings.bokehRadius <= 0.0f)
        return float4(0.0f, 0.0f, 0.0f, 0.0f);

    // 半解像度で描いているので、半径は半解像度のピクセルで数える
    float2 pixelToUV = 2.0f / gFrameData.screenResolution.xy;
    float bokehRadius = cocSize * gDoFSettings.bokehRadius;
    float randomAngle = InterleavedGradientNoise(input.position.xy) * 2.0f * PI;

    float3 colorSum = float3(0.0f, 0.0f, 0.0f);
    float totalWeight = 0.0f;
    for (int i = 0; i < kSampleCount; ++i)
    {
        float theta = i * kGoldenAngle + randomAngle;
        float radius = sqrt(float(i) / kSampleCount);
        float2 sampleUV = input.uv + float2(cos(theta), sin(theta)) * (radius * bokehRadius) * pixelToUV;
        float3 sampleColor = gSceneTexture.SampleLevel(gSampler, sampleUV, 0).rgb;

        // 円の縁を少しぼかし、明るいところを強めて玉ぼけにする
        float highlight = max(CalculateLuminance(sampleColor) - gDoFSettings.bokehHighlightThreshold, 0.0f);
        float weight = smoothstep(1.0f, 0.8f, radius) * (1.0f + highlight * highlight * gDoFSettings.bokehHighlightIntensity);

        colorSum += sampleColor * weight;
        totalWeight += weight;
    }

    // ピントの境目で急に切り替わらないようにする
    return float4(colorSum / max(totalWeight, kEpsilon), smoothstep(0.0f, 0.2f, cocSize));
}
