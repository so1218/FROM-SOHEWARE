#include "FullScreenQuad.hlsli"
#include "ShaderConstants.hlsli"

ConstantBuffer<SSRSettings> gSSRSettings : register(b0);
ConstantBuffer<FrameData> gFrameData : register(b1);

Texture2D<float4> gSceneTexture : register(t0); // 元のシーン画像
Texture2D<float4> gNormalTexture : register(t1); // G-Buffer: 法線
Texture2D<float> gDepthTexture : register(t2); // G-Buffer: 深度
Texture2D<float4> gMaterialTexture : register(t3); // G-Buffer: R=メタルネス, G=ラフネス

SamplerState gClampSampler : register(s0);

// ビュー空間座標を復元する関数
float3 GetViewPos(float2 uv, float depth)
{
    float x = uv.x * 2.0f - 1.0f;
    float y = (1.0f - uv.y) * 2.0f - 1.0f;
    float4 clipPos = float4(x, y, depth, 1.0f);
    float4 viewPos = mul(clipPos, gFrameData.invProjMatrix);
    return viewPos.xyz / viewPos.w;
}

// ハッシュ関数（ジッター用）
float Hash(float2 uv)
{
    return frac(sin(dot(uv, float2(12.9898, 78.233))) * 43758.5453);
}

float4 main(VSOutput input) : SV_TARGET
{
    float depth = gDepthTexture.SampleLevel(gClampSampler, input.uv, 0);
    if (depth >= 1.0f)
        return float4(0.0f, 0.0f, 0.0f, 0.0f);

    float4 material = gMaterialTexture.SampleLevel(gClampSampler, input.uv, 0);
    float metalness = material.r;
    float roughness = material.g;

    if (roughness > 0.8f && metalness < 0.1f)
    {
        return float4(0.0f, 0.0f, 0.0f, 0.0f);
    }

    float3 viewPos = GetViewPos(input.uv, depth);
    float3 worldNormal = gNormalTexture.SampleLevel(gClampSampler, input.uv, 0).xyz;
    float3 viewNormal = normalize(mul(worldNormal, (float3x3)gFrameData.viewMatrix));
    float3 viewDir = normalize(viewPos);
    float3 reflectDir = normalize(reflect(viewDir, viewNormal));

    // カメラ方向へのフェード
    float viewFade = smoothstep(0.0f, 0.2f, reflectDir.z);
    if (viewFade <= 0.0f) return float4(0, 0, 0, 0);

    // ジッター（ノイズ）
    // ピクセルごとにレイの開始位置をランダムにずらし、アーティファクトを防ぐ
    float jitter = Hash(input.uv);
    float bias = 0.05f; 
    // stepSizeの範囲内でランダムに開始位置をずらす
    float3 rayPos = viewPos + (viewNormal * bias) + (reflectDir * gSSRSettings.stepSize * jitter);
    
    float2 hitUV = 0;
    float hitAlpha = 0;
    float rayDistance = 0;

    for (int i = 0; i < gSSRSettings.maxSteps; ++i)
    {
        float3 lastRayPos = rayPos;
        rayPos += reflectDir * gSSRSettings.stepSize;
        rayDistance += gSSRSettings.stepSize;

        float4 offsetPos = mul(float4(rayPos, 1.0f), gFrameData.projectionMatrix);
        offsetPos.xyz /= offsetPos.w;
        float2 sampleUV = float2(offsetPos.x * 0.5f + 0.5f, 1.0f - (offsetPos.y * 0.5f + 0.5f));
        if (sampleUV.x < 0.0f || sampleUV.x > 1.0f || sampleUV.y < 0.0f || sampleUV.y > 1.0f) break;

        float sampleDepth = gDepthTexture.SampleLevel(gClampSampler, sampleUV, 0);
        float sampleZ = GetViewPos(sampleUV, sampleDepth).z;
        float depthDiff = rayPos.z - sampleZ;

        if (depthDiff > 0.0f && depthDiff < gSSRSettings.thickness)
        {
            // 二分探索
            float3 minPos = lastRayPos;
            float3 maxPos = rayPos;
            float3 midPos = 0;
            
            // ループで最適化
            [unroll(10)]
            for (int j = 0; j < 10; ++j)
            {
                midPos = lerp(minPos, maxPos, 0.5f);
                float4 midClip = mul(float4(midPos, 1.0f), gFrameData.projectionMatrix);
                midClip.xyz /= midClip.w;
                float2 midUV = float2(midClip.x * 0.5f + 0.5f, 1.0f - (midClip.y * 0.5f + 0.5f));
                
                // ミップマップ0番で正確な深度を取得
                float mDepth = gDepthTexture.SampleLevel(gClampSampler, midUV, 0);
                float mZ = GetViewPos(midUV, mDepth).z;
                
                if (midPos.z > mZ) maxPos = midPos;
                else minPos = midPos;
            }

            // 追い込んだ最終結果(midPos)から、改めてUVと深度差を計算
            float4 finalClip = mul(float4(midPos, 1.0f), gFrameData.projectionMatrix);
            finalClip.xyz /= finalClip.w;
            hitUV = float2(finalClip.x * 0.5f + 0.5f, 1.0f - (finalClip.y * 0.5f + 0.5f));
            
            // 最終地点での深度を再サンプリング
            float finalDepth = gDepthTexture.SampleLevel(gClampSampler, hitUV, 0);
            float finalZ = GetViewPos(hitUV, finalDepth).z;
            // 最終的な深度差を計算しなおす
            float finalDepthDiff = midPos.z - finalZ;

            // フェード処理
            hitAlpha = saturate(1.0f - (finalDepthDiff / gSSRSettings.thickness));
            
            // 画面端フェード
            float2 fadeUV = smoothstep(0.0f, 0.1f, hitUV) * smoothstep(1.0f, 0.9f, hitUV);
            hitAlpha *= fadeUV.x * fadeUV.y;

            // コンタクトフェード（ジッターを入れたので距離も少し補正）
            hitAlpha *= smoothstep(0.0f, gSSRSettings.stepSize * 1.5f, rayDistance);

            break;
        }
    }

    hitAlpha *= viewFade;
    if (hitAlpha <= 0.0f) return float4(0, 0, 0, 0);
    // ラフネスに応じたMipMapサンプリング
    float maxMipLevel = 0.0f; // テクスチャの最大Mipレベル
    float mipLevel = roughness * maxMipLevel;
    
    float3 reflectionColor = gSceneTexture.SampleLevel(gClampSampler, hitUV, mipLevel).rgb;

    float fresnel = pow(1.0f - max(dot(viewNormal, -viewDir), 0.0f), 5.0f);
    
    // Intensityの計算
    float f0 = lerp(0.04f, 1.0f, metalness);
    float reflectionIntensity = lerp(f0, 1.0f, fresnel) * (1.0f - roughness);

    return float4(reflectionColor, hitAlpha * reflectionIntensity);
}