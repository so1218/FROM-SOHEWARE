#include "ShaderConstants.hlsli"

ConstantBuffer<SSRSettings> gSSRSettings : register(b0);
ConstantBuffer<FrameData> gFrameData : register(b1);

Texture2D<float4> gSceneTexture : register(t0);
Texture2D<float4> gNormalTexture : register(t1);
Texture2D<float> gDepthTexture : register(t2);
Texture2D<float4> gMaterialTexture : register(t3);
SamplerState gClampSampler : register(s0);

// 書き込み用テクスチャ (UAV)
RWTexture2D<float4> gOutReflection : register(u0);

float3 GetViewPos(float2 uv, float depth)
{
    float x = uv.x * 2.0f - 1.0f;
    float y = (1.0f - uv.y) * 2.0f - 1.0f;
    float4 clipPos = float4(x, y, depth, 1.0f);
    float4 viewPos = mul(clipPos, gFrameData.invProjMatrix);
    return viewPos.xyz / viewPos.w;
}

[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 pixelPos = dispatchThreadID.xy;
    
    uint width, height;
    gOutReflection.GetDimensions(width, height);
    if (pixelPos.x >= width || pixelPos.y >= height)
        return;

    float2 uv = (pixelPos + 0.5f) / float2(width, height);

    // -------------------------------------------------------------------------
    // 1. 高速アーリーアウト (不要なピクセルを即死させてGPU負荷をゼロにする)
    // -------------------------------------------------------------------------
    float depth = gDepthTexture.SampleLevel(gClampSampler, uv, 0);
    if (depth >= 1.0f) // 背景・空
    {
        gOutReflection[pixelPos] = float4(0, 0, 0, 0);
        return;
    }

    float4 material = gMaterialTexture.SampleLevel(gClampSampler, uv, 0);
    float metalness = material.r;
    float roughness = material.g;

    // 反射がほぼ起きない粗い材質はレイマーチをスキップ
    if (roughness > 0.8f)
    {
        gOutReflection[pixelPos] = float4(0, 0, 0, 0);
        return;
    }

    // -------------------------------------------------------------------------
    // 2. 座標変換とレイの準備
    // -------------------------------------------------------------------------
    float3 viewPos = GetViewPos(uv, depth);
    float3 worldNormal = gNormalTexture.SampleLevel(gClampSampler, uv, 0).xyz;
    float3 viewNormal = normalize(mul(worldNormal, (float3x3) gFrameData.viewMatrix));
    float3 viewDir = normalize(viewPos);
    float3 reflectDir = reflect(viewDir, viewNormal);

    // -------------------------------------------------------------------------
    // 3. DDA用: ループ外でUV空間の移動量を一括計算 (行列計算をループから排除)
    // -------------------------------------------------------------------------
    float3 viewStart = viewPos + viewNormal * 0.02f;
    float3 viewEnd = viewStart + reflectDir * gSSRSettings.maxDistance;

    float4 clipStart = mul(float4(viewStart, 1.0f), gFrameData.projectionMatrix);
    float4 clipEnd = mul(float4(viewEnd, 1.0f), gFrameData.projectionMatrix);

    clipStart.xyz /= clipStart.w;
    clipEnd.xyz /= clipEnd.w;

    float3 uvzStart = float3(clipStart.x * 0.5f + 0.5f, 1.0f - (clipStart.y * 0.5f + 0.5f), clipStart.z);
    float3 uvzEnd = float3(clipEnd.x * 0.5f + 0.5f, 1.0f - (clipEnd.y * 0.5f + 0.5f), clipEnd.z);

    float3 uvzStep = (uvzEnd - uvzStart) / (float) gSSRSettings.maxSteps;
    float3 currentUVZ = uvzStart;

    float2 hitUV = 0;
    float hitAlpha = 0;
    float rayDistance = 0.0f;

    // -------------------------------------------------------------------------
    // 4. 超高速レイマーチング (ループ内はただの足し算)
    // -------------------------------------------------------------------------
    for (int i = 0; i < gSSRSettings.maxSteps; ++i)
    {
        float3 lastUVZ = currentUVZ;
        currentUVZ += uvzStep;

        if (any(currentUVZ.xy < 0.0f) || any(currentUVZ.xy > 1.0f))
            break;

        float sDepth = gDepthTexture.SampleLevel(gClampSampler, currentUVZ.xy, 0);
        
        // View Zを復元して判定 (非線形Depthの誤差を防ぐ)
        float sZ = GetViewPos(currentUVZ.xy, sDepth).z;
        float rayZ = GetViewPos(currentUVZ.xy, currentUVZ.z).z;
        float depthDiff = rayZ - sZ;

        // 衝突判定
        if (depthDiff > 0.0f && depthDiff < gSSRSettings.thickness)
        {
            // 二分探索 (UV空間で高速補間)
            float3 minUVZ = lastUVZ;
            float3 maxUVZ = currentUVZ;
            float3 midUVZ = 0;

            [unroll(8)]
            for (int j = 0; j < 8; ++j)
            {
                midUVZ = lerp(minUVZ, maxUVZ, 0.5f);
                float mDepth = gDepthTexture.SampleLevel(gClampSampler, midUVZ.xy, 0);
                float mZ = GetViewPos(midUVZ.xy, mDepth).z;
                float mRayZ = GetViewPos(midUVZ.xy, midUVZ.z).z;

                if (mRayZ > mZ)
                    maxUVZ = midUVZ;
                else
                    minUVZ = midUVZ;
            }

            float3 hitNormal = gNormalTexture.SampleLevel(gClampSampler, midUVZ.xy, 0).xyz;
            float3 vHitNormal = normalize(mul(hitNormal, (float3x3) gFrameData.viewMatrix));

            if (dot(vHitNormal, reflectDir) < 0.0f)
            {
                hitUV = midUVZ.xy;
                rayDistance = length(GetViewPos(midUVZ.xy, midUVZ.z) - viewPos);
                hitAlpha = smoothstep(0.0f, gSSRSettings.stepSize * 2.0f, rayDistance);
                break;
            }
        }
    }

    // -------------------------------------------------------------------------
    // 5. フェード処理と出力
    // -------------------------------------------------------------------------
    float2 edgeFade = min(hitUV, 1.0f - hitUV) * 10.0f;
    hitAlpha *= saturate(edgeFade.x) * saturate(edgeFade.y);
    hitAlpha *= (1.0f - saturate(rayDistance / gSSRSettings.maxDistance));

    if (hitAlpha <= 0.0f)
    {
        gOutReflection[pixelPos] = float4(0, 0, 0, 0);
        return;
    }

    float3 f0 = lerp(float3(0.04, 0.04, 0.04), gSceneTexture.SampleLevel(gClampSampler, uv, 0).rgb, metalness);
    float reflectionMip = roughness * 5.0f;
    float3 reflectionColor = gSceneTexture.SampleLevel(gClampSampler, hitUV, reflectionMip).rgb;
    float3 fresnel = f0 + (1.0 - f0) * pow(1.0 - max(dot(viewNormal, -viewDir), 0.0), 5.0);

    // UAVへ直接書き込み
    gOutReflection[pixelPos] = float4(reflectionColor * fresnel, hitAlpha);
}