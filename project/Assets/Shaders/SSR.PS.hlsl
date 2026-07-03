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

    // 反射の強さの基本 (F0)
    float3 f0 = lerp(float3(0.04, 0.04, 0.04), gSceneTexture.SampleLevel(gClampSampler, input.uv, 0).rgb, metalness);

    float3 viewPos = GetViewPos(input.uv, depth);
    float3 worldNormal = gNormalTexture.SampleLevel(gClampSampler, input.uv, 0).xyz;
    float3 viewNormal = normalize(mul(worldNormal, (float3x3) gFrameData.viewMatrix));
    float3 viewDir = normalize(viewPos);
    float3 reflectDir = reflect(viewDir, viewNormal);

    // レイの初期位置(セルフシャドウ防止のため法線方向に浮かせる)
    float3 rayPos = viewPos + viewNormal * 0.02f;
    float rayDistance = 0.0f;
    float2 hitUV = 0;
    float hitAlpha = 0;

// レイマーチング部
    for (int i = 0; i < gSSRSettings.maxSteps; ++i)
    {
        float3 lastRayPos = rayPos; // 今の位置を1歩前として保存
        
        float currentStep = gSSRSettings.stepSize * (1.0f + i * 0.1f);
        rayPos += reflectDir * currentStep;
        rayDistance += currentStep; // 距離を加算

        float4 offsetPos = mul(float4(rayPos, 1.0f), gFrameData.projectionMatrix);
        offsetPos.xyz /= offsetPos.w;
        float2 sampleUV = float2(offsetPos.x * 0.5f + 0.5f, 1.0f - (offsetPos.y * 0.5f + 0.5f));

        if (any(sampleUV < 0) || any(sampleUV > 1))
            break;

        float sDepth = gDepthTexture.SampleLevel(gClampSampler, sampleUV, 0);
        float sZ = GetViewPos(sampleUV, sDepth).z;
        float depthDiff = rayPos.z - sZ;

        // 衝突判定
        if (depthDiff > 0.0f && depthDiff < gSSRSettings.thickness)
        {
            // 二分探索
            float3 minPos = lastRayPos;
            float3 maxPos = rayPos;
            float3 midPos = 0;

            [unroll(10)]
            for (int j = 0; j < 10; ++j)
            {
                midPos = lerp(minPos, maxPos, 0.5f);
                float4 midClip = mul(float4(midPos, 1.0f), gFrameData.projectionMatrix);
                midClip.xyz /= midClip.w;
                float2 midUV = float2(midClip.x * 0.5f + 0.5f, 1.0f - (midClip.y * 0.5f + 0.5f));
                float mDepth = gDepthTexture.SampleLevel(gClampSampler, midUV, 0);
                float mZ = GetViewPos(midUV, mDepth).z;
                if (midPos.z > mZ)
                    maxPos = midPos;
                else
                    minPos = midPos;
            }

            // 衝突後の最終チェック
            float4 finalClip = mul(float4(midPos, 1.0f), gFrameData.projectionMatrix);
            finalClip.xyz /= finalClip.w;
            float2 finalUV = float2(finalClip.x * 0.5f + 0.5f, 1.0f - (finalClip.y * 0.5f + 0.5f));

            float3 hitNormal = gNormalTexture.SampleLevel(gClampSampler, finalUV, 0).xyz;
            float3 vHitNormal = normalize(mul(hitNormal, (float3x3) gFrameData.viewMatrix));

            // 法線チェック(レイの進行方向と面が向き合っているか)
            if (dot(vHitNormal, reflectDir) < 0.0f)
            {
                hitUV = finalUV;
                // コンタクトフェード（足元のノイズ消し）
                hitAlpha = smoothstep(0.0f, gSSRSettings.stepSize * 2.0f, rayDistance);
                break; // ループを抜ける
            }
        }
    }

    // フェード処理
    // 画面端フェード
    float2 edgeFade = min(hitUV, 1.0f - hitUV) * 10.0f;
    hitAlpha *= saturate(edgeFade.x) * saturate(edgeFade.y);
    
    // 反射距離によるフェード（遠すぎる反射を薄く）
    hitAlpha *= (1.0f - saturate(length(rayPos - viewPos) / gSSRSettings.maxDistance));

    if (hitAlpha <= 0.0f)
        return float4(0, 0, 0, 0);

    // MipMapを 11.0f (2048px想定) 等に設定
    float reflectionMip = roughness * 5.0f;
    float3 reflectionColor = gSceneTexture.SampleLevel(gClampSampler, hitUV, reflectionMip).rgb;

    // フレネル
    float3 fresnel = f0 + (1.0 - f0) * pow(1.0 - max(dot(viewNormal, -viewDir), 0.0), 5.0);

    return float4(reflectionColor * fresnel, hitAlpha);
}