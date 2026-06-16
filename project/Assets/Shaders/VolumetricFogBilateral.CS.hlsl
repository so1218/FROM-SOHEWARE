#include "ShaderConstants.hlsli"

Texture2D<float4> gRawFogTexture : register(t0);
Texture2D<float> gDepthTexture : register(t1);

// ぼかし処理が終わった最終結果
RWTexture2D<float4> gFilteredFog : register(u0);

ConstantBuffer<FogBilateralSettings> gFogBilateralSettings : register(b0);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint width, height;
    gFilteredFog.GetDimensions(width, height);
    if (DTid.x >= width || DTid.y >= height)
        return;

    // 1. 中心ピクセルの情報を取得
    int3 centerPos = int3(DTid.xy, 0);
    float4 centerFog = gRawFogTexture.Load(centerPos);
    float centerDepth = gDepthTexture.Load(centerPos).r;

    float4 resultColor = float4(0, 0, 0, 0);
    float totalWeight = 0.0f;

    // 定数バッファから値を取得
    int radius = gFogBilateralSettings.blurRadius;
    
    // 空間ウェイト計算用の定数
    float spatialCoeff = 1.0f / (2.0f * gFogBilateralSettings.spatialSigma * gFogBilateralSettings.spatialSigma);
    
    // 深度ウェイト計算用の定数（0割り防止のため、念のため微小な下限値を設ける）
    float safeDepthSigma = max(gFogBilateralSettings.depthSigma, 0.00001f);
    float depthCoeff = 1.0f / (2.0f * safeDepthSigma * safeDepthSigma);

    // 2. 周辺ピクセルをサンプリングして合成
    for (int y = -radius; y <= radius; ++y)
    {
        for (int x = -radius; x <= radius; ++x)
        {
            // サンプル座標の計算（画面外にはみ出さないようにクランプ）
            int2 sampleCoord = clamp(int2(DTid.x + x, DTid.y + y), int2(0, 0), int2(width - 1, height - 1));
            int3 samplePos = int3(sampleCoord, 0);

            // 周辺ピクセルの情報を取得
            float4 sampleFog = gRawFogTexture.Load(samplePos);
            float sampleDepth = gDepthTexture.Load(samplePos).r;

            // --- ウェイト（重み）の計算 ---
            
            // 空間ウェイト（中心から遠いピクセルほど影響力を下げる）
            float distSq = (float) (x * x + y * y);
            float spatialWeight = exp(-distSq * spatialCoeff);

            // 深度ウェイト（中心ピクセルと深度が離れているほど影響力をゼロに近づける）
            float depthDiff = abs(centerDepth - sampleDepth);
            float depthWeight = exp(-(depthDiff * depthDiff) * depthCoeff);

            // 最終的なウェイト
            float weight = spatialWeight * depthWeight;

            // 結果に加算
            resultColor += sampleFog * weight;
            totalWeight += weight;
        }
    }

    // 総ウェイトで割って平均化し、出力テクスチャに書き込む
    gFilteredFog[DTid.xy] = resultColor / max(totalWeight, 0.00001f);
}