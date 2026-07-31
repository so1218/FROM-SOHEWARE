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

    int3 centerPos = int3(DTid.xy, 0);
    float4 centerFog = gRawFogTexture.Load(centerPos);
    float centerDepth = gDepthTexture.Load(centerPos).r;

    float4 resultColor = 0.0f;
    float totalWeight = 0.0f;
    int radius = gFogBilateralSettings.blurRadius;
    
    // Gauss関数の指数部係数: 1.0 / (2.0 * sigma^2)
    float spatialCoeff = 1.0f / (2.0f * gFogBilateralSettings.spatialSigma * gFogBilateralSettings.spatialSigma);
   
    float safeDepthSigma = max(gFogBilateralSettings.depthSigma, kExtinctionEpsilon);
    float depthCoeff = 1.0f / (2.0f * safeDepthSigma * safeDepthSigma);

    for (int y = -radius; y <= radius; ++y)
    {
        for (int x = -radius; x <= radius; ++x)
        {
            // サンプル座標を画面内にクランプ
            int2 sampleCoord = clamp(int2(DTid.x + x, DTid.y + y), 0, int2(width - 1, height - 1));
            int3 samplePos = int3(sampleCoord, 0);

            float4 sampleFog = gRawFogTexture.Load(samplePos);
            float sampleDepth = gDepthTexture.Load(samplePos).r;

            // 空間ウェイト計算
            float distSq = float(x * x + y * y);
            float spatialWeight = exp(-distSq * spatialCoeff);

            // 深度ウェイト計算（エッジ保持）
            float depthDiff = centerDepth - sampleDepth;
            float depthWeight = exp(-(depthDiff * depthDiff) * depthCoeff);

            float weight = spatialWeight * depthWeight;

            resultColor += sampleFog * weight;
            totalWeight += weight;
        }
    }

    gFilteredFog[DTid.xy] = resultColor / max(totalWeight, kExtinctionEpsilon);
}