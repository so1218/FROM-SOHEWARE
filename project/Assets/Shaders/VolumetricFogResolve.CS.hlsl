#include "ShaderConstants.hlsli"

Texture2D<float> gDepthTexture : register(t0);
Texture3D<float4> gVoxelAccumulate : register(t1);
Texture2D<float4> gHistoryTexture : register(t2); // 前フレームの出力結果
SamplerState gLinearSampler : register(s0);
RWTexture2D<float4> gOutput : register(u0);

ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<VolumetricFogSettings> gFogSettings : register(b2);

// ディザリング用の高速な疑似乱数ノイズ（Interleaved Gradient Noise）
// ディザリング用の高速な疑似乱数ノイズ（Interleaved Gradient Noise）
float InterleavedGradientNoise(float2 pixelCoord, uint frameIndex)
{
    pixelCoord += float2(frameIndex * 5.588238f, frameIndex * 5.588238f);
    float3 magic = float3(0.06711056f, 0.00583715f, 52.9829189f);
    return frac(magic.z * frac(dot(pixelCoord, magic.xy)));
}

// 輝度（Luminance）の計算
float GetLuminance(float3 color)
{
    return dot(color, float3(0.2126f, 0.7152f, 0.0722f));
}

// ============================================================================
// メインシェーダー
// ============================================================================
[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint width, height;
    gOutput.GetDimensions(width, height);
    
    if (DTid.x >= width || DTid.y >= height)
        return;

    float2 texelSize = 1.0f / float2(width, height);
    float2 uv = (float2(DTid.xy) + 0.5f) * texelSize;

    // ------------------------------------------------------------------------
    // 1. 深度とワールド座標の復元
    // ------------------------------------------------------------------------
    float depthVal = gDepthTexture.SampleLevel(gLinearSampler, uv, 0).r;
    float4 clipPos = float4(uv.x * 2.0f - 1.0f, (1.0f - uv.y) * 2.0f - 1.0f, depthVal, 1.0f);
    float4 worldPosFull = mul(clipPos, gFrameData.invViewProj);
    float3 worldPos = worldPosFull.xyz / worldPosFull.w;

    float nearZ = max(gFrameData.nearClip, 0.1f);
    float farZ = min(gFrameData.farClip, gFogSettings.maxDistance);
    float rayLength = length(worldPos - gFrameData.cameraWorldPosition);
    float clampedDistance = clamp(rayLength, nearZ, farZ);

    // ------------------------------------------------------------------------
    // 2. ジッタ付きサンプリング (バンディング対策)
    // ------------------------------------------------------------------------
    float dither = InterleavedGradientNoise(DTid.xy, gFrameData.frameIndex);
    float sliceRes = 64.0f; // ※将来的に定数バッファからの取得を推奨
    float linearStep = log2(clampedDistance / nearZ) / log2(farZ / nearZ);
    float zSlice = saturate(linearStep + (dither - 0.5f) * (1.0f / sliceRes));
    
    // 現在のフォグを取得
    float4 currentFog = gVoxelAccumulate.SampleLevel(gLinearSampler, float3(uv, zSlice), 0);

    // ------------------------------------------------------------------------
    // 3. リプロジェクション (前フレーム履歴の取得)
    // ------------------------------------------------------------------------
    float4 prevClipPos = mul(float4(worldPos, 1.0f), gFrameData.prevViewProj);
    prevClipPos.xyz /= prevClipPos.w;
    float2 prevUV = prevClipPos.xy * float2(0.5f, -0.5f) + 0.5f;

    float4 historyFog = gHistoryTexture.SampleLevel(gLinearSampler, prevUV, 0);

    // ------------------------------------------------------------------------
    // 4. Variance Clipping (ゴースト対策: 3x3 近傍統計によるクランプ)
    // ------------------------------------------------------------------------
    float3 m1 = 0.0f;
    float3 m2 = 0.0f;
    
    [unroll]
    for (int y = -1; y <= 1; ++y)
    {
        [unroll]
        for (int x = -1; x <= 1; ++x)
        {
            float3 neighbor = gVoxelAccumulate.SampleLevel(
                gLinearSampler,
                float3(uv + float2(x, y) * texelSize, zSlice),
                0
            ).rgb;
            
            m1 += neighbor;
            m2 += neighbor * neighbor;
        }
    }
    
    float3 mean = m1 / 9.0f;
    float3 variance = max(m2 / 9.0f - (mean * mean), 0.00001f);
    float3 stdDev = sqrt(variance);
    
    // クランプ範囲 (3.0シグマ)
    float3 minColor = mean - stdDev * 3.0f;
    float3 maxColor = mean + stdDev * 3.0f;
    
    // 履歴を現在の統計範囲にソフトクランプ
    float3 clampedHistory = clamp(historyFog.rgb, minColor, maxColor);
    historyFog.rgb = lerp(historyFog.rgb, clampedHistory, 0.8f);

    // ------------------------------------------------------------------------
    // 5. 履歴の棄却判定とウェイト計算
    // ------------------------------------------------------------------------
    bool isOffscreen = any(prevUV < 0.0f) || any(prevUV > 1.0f);
    float blendAlpha = isOffscreen ? 1.0f : 0.05f;

    // ------------------------------------------------------------------------
    // 6. 最終合成 (Karis Average: Fireflies/チリチリ対策)
    // ------------------------------------------------------------------------
    float currentLum = GetLuminance(currentFog.rgb);
    float historyLum = GetLuminance(historyFog.rgb);
    
    float weight = 1.0f / (1.0f + currentLum);
    float historyWeight = 1.0f / (1.0f + historyLum);
    
    float4 result = (historyFog * historyWeight * (1.0f - blendAlpha) + currentFog * weight * blendAlpha) /
                    max(historyWeight * (1.0f - blendAlpha) + weight * blendAlpha, 0.00001f); // ゼロ除算保護

    // 出力
    gOutput[DTid.xy] = result;
}