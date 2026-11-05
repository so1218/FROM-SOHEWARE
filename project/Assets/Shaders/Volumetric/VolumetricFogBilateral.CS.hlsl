#include "Common/ShaderConstants.hlsli"
#include "Common/MathUtils.hlsli"

Texture2D<float4> gRawFogTexture : register(t0);
Texture2D<float> gDepthTexture : register(t1);
RWTexture2D<float4> gFilteredFog : register(u0);

ConstantBuffer<FogBilateralSettings> gFogBilateralSettings : register(b0);

// 深度が近いピクセルだけ混ぜて、輪郭で霧がにじまないようにする
[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    uint outputWidth, outputHeight;
    gFilteredFog.GetDimensions(outputWidth, outputHeight);
    if (any(dispatchThreadId.xy >= uint2(outputWidth, outputHeight)))
        return;

    // 霧は半解像度なので、深度は大きさの比で読む
    uint depthWidth, depthHeight;
    gDepthTexture.GetDimensions(depthWidth, depthHeight);
    float2 depthScale = float2(depthWidth, depthHeight) / float2(outputWidth, outputHeight);
    int2 maxCoord = int2(outputWidth, outputHeight) - 1;
    int2 maxDepthCoord = int2(depthWidth, depthHeight) - 1;

    float centerDepth = gDepthTexture.Load(int3(min(int2(dispatchThreadId.xy * depthScale), maxDepthCoord), 0));

    int radius = gFogBilateralSettings.blurRadius;
    float spatialFactor = 1.0f / (2.0f * gFogBilateralSettings.spatialSigma * gFogBilateralSettings.spatialSigma);
    float depthSigma = max(gFogBilateralSettings.depthSigma, kEpsilon);
    float depthFactor = 1.0f / (2.0f * depthSigma * depthSigma);

    float4 sum = 0.0f;
    float totalWeight = 0.0f;
    for (int y = -radius; y <= radius; ++y)
    {
        for (int x = -radius; x <= radius; ++x)
        {
            int2 sampleCoord = clamp(int2(dispatchThreadId.xy) + int2(x, y), 0, maxCoord);
            float4 fog = gRawFogTexture.Load(int3(sampleCoord, 0));
            float sampleDepth = gDepthTexture.Load(int3(min(int2(sampleCoord * depthScale), maxDepthCoord), 0));

            float depthDifference = centerDepth - sampleDepth;
            float weight = exp(-float(x * x + y * y) * spatialFactor) * exp(-depthDifference * depthDifference * depthFactor);

            sum += fog * weight;
            totalWeight += weight;
        }
    }

    gFilteredFog[dispatchThreadId.xy] = sum / max(totalWeight, kEpsilon);
}
