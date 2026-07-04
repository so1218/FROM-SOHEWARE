#include "ShaderConstants.hlsli"

// 入力テクスチャ
Texture2D<float4> gNoisySSRTexture : register(t0); // 先ほどのResolveパスの出力
Texture2D<float4> gNormalTexture : register(t1); // G-Buffer: 法線
Texture2D<float> gDepthTexture : register(t2); // G-Buffer: 深度
Texture2D<float4> gMaterialTexture : register(t3); // G-Buffer: R=メタルネス, G=ラフネス

// 出力テクスチャ
RWTexture2D<float4> gOutDenoisedSSR : register(u0);

// デノイズ用のパラメータ群（※本来はConstantBufferから渡すのが理想です）
static const float gDepthThreshold = 0.02f; // 深度の許容差（大きくすると段差を越えてぼやける）
static const float gNormalPower = 32.0f; // 法線の許容シビアさ（大きいほど少しの角度差で弾く）
static const float gMaxBlurRadius = 3.0f; // 最大サンプリング半径

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint width, height;
    gOutDenoisedSSR.GetDimensions(width, height);
    if (DTid.x >= width || DTid.y >= height)
        return;

    int2 centerCoord = DTid.xy;
    float4 centerSSR = gNoisySSRTexture.Load(int3(centerCoord, 0));
    
    // 早期リターン：反射マスク(Alpha)が0なら計算不要
    if (centerSSR.a <= 0.001f)
    {
        gOutDenoisedSSR[centerCoord] = float4(0, 0, 0, 0);
        return;
    }

    float centerDepth = gDepthTexture.Load(int3(centerCoord, 0));
    float3 centerNormal = gNormalTexture.Load(int3(centerCoord, 0)).xyz;
    float roughness = gMaterialTexture.Load(int3(centerCoord, 0)).g;

    // 軽量化：完全な鏡面（ラフネスがほぼ0）ならノイズは出ていないので、そのまま出力
    if (roughness < 0.02f)
    {
        gOutDenoisedSSR[centerCoord] = centerSSR;
        return;
    }

    // ラフネスに応じてブラーのサンプリング半径を動的に決定（最大半径3=7x7ピクセル）
    int radius = (int) ceil(gMaxBlurRadius * roughness);
    radius = clamp(radius, 1, 3);

    float4 colorSum = float4(0, 0, 0, 0);
    float weightSum = 0.0f;

    // --- バイラテラル・フィルタリング ---
    for (int y = -radius; y <= radius; ++y)
    {
        for (int x = -radius; x <= radius; ++x)
        {
            int2 sampleCoord = centerCoord + int2(x, y);
            
            // 画面外アクセス防止
            sampleCoord = clamp(sampleCoord, int2(0, 0), int2(width - 1, height - 1));

            float sampleDepth = gDepthTexture.Load(int3(sampleCoord, 0));
            float3 sampleNormal = gNormalTexture.Load(int3(sampleCoord, 0)).xyz;
            float4 sampleSSR = gNoisySSRTexture.Load(int3(sampleCoord, 0));

            // 1. 空間ウェイト (距離が近いほど重み大)
            // Gaussian近似
            float spatialWeight = exp(-float(x * x + y * y) / (2.0f * (radius * radius / 4.0f)));

            // 2. 深度ウェイト (段差が少ないほど重み大)
            float depthDiff = abs(centerDepth - sampleDepth);
            float depthWeight = exp(-(depthDiff * depthDiff) / (2.0f * gDepthThreshold * gDepthThreshold));

            // 3. 法線ウェイト (向きが同じほど重み大)
            float normalDot = max(dot(centerNormal, sampleNormal), 0.0f);
            float normalWeight = pow(normalDot, gNormalPower);

            // 総合ウェイトの算出
            float finalWeight = spatialWeight * depthWeight * normalWeight;

            colorSum += sampleSSR * finalWeight;
            weightSum += finalWeight;
        }
    }

    // ウェイトの合計で割って正規化
    float4 finalColor = (weightSum > 0.0001f) ? (colorSum / weightSum) : centerSSR;
    
    gOutDenoisedSSR[centerCoord] = finalColor;
}