#include "ShaderConstants.hlsli"

// 入力テクスチャ
Texture2D<float4> gNoisySSRTexture : register(t0); // 先ほどのResolveパスの出力
Texture2D<float4> gNormalTexture : register(t1); // G-Buffer: 法線
Texture2D<float> gDepthTexture : register(t2); // G-Buffer: 深度
Texture2D<float4> gMaterialTexture : register(t3); // G-Buffer: R=メタルネス, G=ラフネス

// 出力テクスチャ
RWTexture2D<float4> gOutDenoisedSSR : register(u0);

// ※深度がリニア(実距離)になったので、閾値を「メートル」感覚で調整します（※本来はConstantBufferから渡すのが理想です）
static const float gDepthThreshold = 0.5f;
static const float gNormalPower = 32.0f;
static const float gMaxBlurRadius = 3.0f;

// Resolveパスと同じ座標復元関数を追加
float3 GetViewPos(float2 uv, float depth)
{
    float x = uv.x * 2.0f - 1.0f;
    float y = (1.0f - uv.y) * 2.0f - 1.0f;
    float4 clipPos = float4(x, y, depth, 1.0f);
    float4 viewPos = mul(clipPos, gFrameData.invProjMatrix);
    return viewPos.xyz / viewPos.w;
}

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint width, height;
    gOutDenoisedSSR.GetDimensions(width, height);
    if (DTid.x >= width || DTid.y >= height)
        return;

    int2 centerCoord = DTid.xy;
    float4 centerSSR = gNoisySSRTexture.Load(int3(centerCoord, 0));
    
    if (centerSSR.a <= 0.001f)
    {
        gOutDenoisedSSR[centerCoord] = float4(0, 0, 0, 0);
        return;
    }

    float centerDepth = gDepthTexture.Load(int3(centerCoord, 0));
    float3 centerNormal = gNormalTexture.Load(int3(centerCoord, 0)).xyz;
    float roughness = gMaterialTexture.Load(int3(centerCoord, 0)).g;

    if (roughness < 0.02f)
    {
        gOutDenoisedSSR[centerCoord] = centerSSR;
        return;
    }

    // 中心の深度をリニアなビューZへ変換
    float2 centerUV = (float2(centerCoord) + 0.5f) / float2(width, height);
    float centerLinearZ = GetViewPos(centerUV, centerDepth).z;

    int radius = (int) ceil(gMaxBlurRadius * roughness);
    radius = clamp(radius, 1, 3);

    float4 colorSum = float4(0, 0, 0, 0);
    float weightSum = 0.0f;

    for (int y = -radius; y <= radius; ++y)
    {
        for (int x = -radius; x <= radius; ++x)
        {
            int2 sampleCoord = centerCoord + int2(x, y);
            sampleCoord = clamp(sampleCoord, int2(0, 0), int2(width - 1, height - 1));

            float sampleDepth = gDepthTexture.Load(int3(sampleCoord, 0));
            float3 sampleNormal = gNormalTexture.Load(int3(sampleCoord, 0)).xyz;
            float4 sampleSSR = gNoisySSRTexture.Load(int3(sampleCoord, 0));

            // サンプルの深度もリニアなビューZへ変換して比較
            float2 sampleUV = (float2(sampleCoord) + 0.5f) / float2(width, height);
            float sampleLinearZ = GetViewPos(sampleUV, sampleDepth).z;

            float spatialWeight = exp(-float(x * x + y * y) / (2.0f * (radius * radius / 4.0f)));
            
            // 線形距離の差分でウェイトを計算
            float depthDiff = abs(centerLinearZ - sampleLinearZ);
            float depthWeight = exp(-(depthDiff * depthDiff) / (2.0f * gDepthThreshold * gDepthThreshold));

            float normalDot = max(dot(centerNormal, sampleNormal), 0.0f);
            float normalWeight = pow(normalDot, gNormalPower);

            float finalWeight = spatialWeight * depthWeight * normalWeight;

            colorSum += sampleSSR * finalWeight;
            weightSum += finalWeight;
        }
    }

    float4 finalColor = (weightSum > 0.0001f) ? (colorSum / weightSum) : centerSSR;
    gOutDenoisedSSR[centerCoord] = finalColor;
}