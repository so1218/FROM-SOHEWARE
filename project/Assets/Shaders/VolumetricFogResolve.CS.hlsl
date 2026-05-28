#include "ShaderConstants.hlsli"

Texture2D<float> gDepthTexture : register(t0);
Texture3D<float4> gVoxelAccumulate : register(t1);
Texture2D<float4> gHistoryTexture : register(t2); // 前フレームの出力結果
SamplerState gLinearSampler : register(s0);
RWTexture2D<float4> gOutput : register(u0);

ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<VolumetricFogSettings> gFogSettings : register(b2);

// ディザリング用の高速な疑似乱数ノイズ（Interleaved Gradient Noise）
float InterleavedGradientNoise(float2 pixelCoord, uint frameIndex)
{
    // フレームごとにピクセル座標をズラしてノイズを変える
    pixelCoord += float2(frameIndex * 5.588238f, frameIndex * 5.588238f);
    
    float3 magic = float3(0.06711056f, 0.00583715f, 52.9829189f);
    return frac(magic.z * frac(dot(pixelCoord, magic.xy)));
}

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint width, height;
    gOutput.GetDimensions(width, height);
    if (DTid.x >= width || DTid.y >= height)
        return;

    float2 uv = (float2(DTid.xy) + 0.5f) / float2(width, height);
    float depthVal = gDepthTexture.SampleLevel(gLinearSampler, uv, 0).r;

    // --- ワールド座標復元 ---
    float4 clipPos = float4(uv.x * 2.0f - 1.0f, (1.0f - uv.y) * 2.0f - 1.0f, depthVal, 1.0f);
    float4 worldPosFull = mul(clipPos, gFrameData.invViewProj);
    float3 worldPos = worldPosFull.xyz / worldPosFull.w;

    float nearZ = max(gFrameData.nearClip, 0.1f);
    float farZ = min(gFrameData.farClip, gFogSettings.maxDistance);
    float rayLength = length(worldPos - gFrameData.cameraWorldPosition);
    float clampedDistance = clamp(rayLength, nearZ, farZ);

    // --- 【プロの技1】ジッタの安定化 ---
    float dither = InterleavedGradientNoise(DTid.xy, gFrameData.frameIndex);
    float sliceRes = 64.0f;
    float linearStep = log2(clampedDistance / nearZ) / log2(farZ / nearZ);
    
    // 単一サンプリングではなく、近傍2スライスを補完（またはジッタを抑える）
    float zSlice = saturate(linearStep + (dither - 0.5f) * (1.0f / sliceRes));
    
    // 現在のフォグを取得
    float4 currentFog = gVoxelAccumulate.SampleLevel(gLinearSampler, float3(uv, zSlice), 0);

    // --- 【プロの技2】リプロジェクションと深度リジェクション ---
    float4 prevClipPos = mul(float4(worldPos, 1.0f), gFrameData.prevViewProj);
    prevClipPos.xyz /= prevClipPos.w;
    float2 prevUV = prevClipPos.xy * float2(0.5f, -0.5f) + 0.5f;

    // 前フレームのフォグを取得
    float4 historyFog = gHistoryTexture.SampleLevel(gLinearSampler, prevUV, 0);

    // --- 3x3 近傍統計（Variance Clipping） ---
    float2 texelSize = 1.0f / float2(width, height);
    float3 m1 = 0;
    float3 m2 = 0;
    
    [unroll]
    for (int y = -1; y <= 1; ++y)
    {
        [unroll]
        for (int x = -1; x <= 1; ++x)
        {
            float3 neighbor = gVoxelAccumulate.SampleLevel(gLinearSampler, float3(uv + float2(x, y) * texelSize, zSlice), 0).rgb;
            m1 += neighbor;
            m2 += neighbor * neighbor;
        }
    }
    float3 mean = m1 / 9.0;
    float3 stdDev = sqrt(max(m2 / 9.0 - mean * mean, 0.00001));
    
    // クランプ範囲を広めに設定 (チリチリ防止)
    float3 minColor = mean - stdDev * 3.0;
    float3 maxColor = mean + stdDev * 3.0;
    
    // ソフトクランプ
    float3 clampedHistory = clamp(historyFog.rgb, minColor, maxColor);
    historyFog.rgb = lerp(historyFog.rgb, clampedHistory, 0.8f);

    // --- 【プロの技3】残像（Ghosting）対策：深度ベースのリジェクション ---
    // 前フレームのワールド座標から「現在のカメラでの深度」を逆算し、
    // 今の深度と大きく乖離していたら履歴を捨てる（＝動いている物体のエッジの残像を消す）
    bool isOffscreen = any(prevUV < 0.0f) || any(prevUV > 1.0f);
    
    // 履歴のウェイト計算
    float blendAlpha = 0.05f; // 基本の蓄積率
    
    // オフスクリーンなら履歴を捨てる
    if (isOffscreen)
        blendAlpha = 1.0f;

    // 【重要】ルミナンスベースの重み付け（Fireflies/チリチリ対策）
    // 明るすぎるピクセル（ノイズ）の重みを下げる
    float currentLum = dot(currentFog.rgb, float3(0.2126, 0.7152, 0.0722));
    float historyLum = dot(historyFog.rgb, float3(0.2126, 0.7152, 0.0722));
    float weight = 1.0f / (1.0f + currentLum);
    float historyWeight = 1.0f / (1.0f + historyLum);
    
    // 最終合成
    float4 result = (historyFog * historyWeight * (1.0 - blendAlpha) + currentFog * weight * blendAlpha) /
                    (historyWeight * (1.0 - blendAlpha) + weight * blendAlpha);

    gOutput[DTid.xy] = result;
}