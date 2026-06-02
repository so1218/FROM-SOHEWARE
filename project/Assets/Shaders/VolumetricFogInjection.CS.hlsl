#include "ShaderConstants.hlsli"
#include "Object3D.hlsli"

Texture2D<float> gDepthTexture : register(t0);
Texture2D<float> gShadowMap : register(t1);
Texture3D<float> gNoiseVolume : register(t2);
SamplerState gSampler : register(s0);
SamplerComparisonState gShadowSampler : register(s1);

// 3Dテクスチャへの出力
RWTexture3D<float4> gVoxelInject : register(u0);

ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<VolumetricFogSettings> gFogSettings : register(b2);

cbuffer PointLights : register(b3)
{
    PointLight gPointLights[MAX_POINT_LIGHTS];
};
cbuffer SpotLights : register(b4)
{
    SpotLight gSpotLights[MAX_SPOT_LIGHTS];
};

// 位相関数
float PhaseFunctionHG(float cosTheta, float g)
{
    float g2 = g * g;
    float denom = 1.0f + g2 - 2.0f * g * cosTheta;
    return (1.0f - g2) / (4.0f * 3.14159265f * pow(max(denom, 0.0001f), 1.5f));
}

float DualPhaseHG(float cosTheta, float g)
{
    float forward = PhaseFunctionHG(cosTheta, g);
    float backward = PhaseFunctionHG(cosTheta, -0.2f);
    return lerp(backward, forward, 0.9f);
}

float InterleavedGradientNoise(float2 pixelCoord, uint frameIndex)
{
    // フレームごとにピクセル座標をズラしてノイズを変える
    pixelCoord += float2(frameIndex * 5.588238f, frameIndex * 5.588238f);
    
    float3 magic = float3(0.06711056f, 0.00583715f, 52.9829189f);
    return frac(magic.z * frac(dot(pixelCoord, magic.xy)));
}

[numthreads(8, 8, 4)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint width, height, depth;
    gVoxelInject.GetDimensions(width, height, depth);
    if (DTid.x >= width || DTid.y >= height || DTid.z >= depth)
        return;

    float nearZ = max(gFrameData.nearClip, 0.1f);
    float farZ = min(gFrameData.farClip, gFogSettings.maxDistance);
    
    float zSlice0 = float(DTid.z) / float(depth);
    float zSlice1 = float(DTid.z + 1.0f) / float(depth);
    
    float viewZ0 = nearZ * pow(farZ / nearZ, zSlice0);
    float viewZ1 = nearZ * pow(farZ / nearZ, zSlice1);
    float voxelThickness = viewZ1 - viewZ0;

    float screenU = (float(DTid.x) + 0.5f) / float(width);
    float screenV = (float(DTid.y) + 0.5f) / float(height);
    float clipX = screenU * 2.0f - 1.0f;
    float clipY = (1.0f - screenV) * 2.0f - 1.0f;
    
    float4 worldTarget = mul(float4(clipX, clipY, 1.0f, 1.0f), gFrameData.invViewProj);
    float3 rayDir = normalize(worldTarget.xyz / worldTarget.w - gFrameData.cameraWorldPosition);
    
    // =========================================================
    // 【追加】シーンの深度（Depth）を取得し、カメラからの距離を計算
    // =========================================================
    float hwDepth = gDepthTexture.SampleLevel(gSampler, float2(screenU, screenV), 0).r;
    float4 sceneWorld = mul(float4(clipX, clipY, hwDepth, 1.0f), gFrameData.invViewProj);
    sceneWorld.xyz /= sceneWorld.w;
    float sceneDist = length(sceneWorld.xyz - gFrameData.cameraWorldPosition);

    // ボクセルの手前側が既にオブジェクトの裏側（地中や壁の裏）にある場合、
    // まるごと計算をスキップして処理を軽くする＆リークを防ぐ
    if (viewZ0 > sceneDist)
    {
        gVoxelInject[DTid.xyz] = float4(0, 0, 0, 0);
        return;
    }
    // =========================================================

    float noiseJitter = InterleavedGradientNoise(float2(DTid.xy), gFrameData.frameIndex);
    
    float3 accumScattering = 0;
    float accumExtinction = 0;
    
    const int NUM_SAMPLES = 2;

    for (int i = 0; i < NUM_SAMPLES; ++i)
    {
        float t = (float(i) + noiseJitter) / float(NUM_SAMPLES);
        float sampleViewZ = viewZ0 + voxelThickness * t;

        // 【追加】サンプリング点がオブジェクトの裏側に食い込んだら計算を無視する
        if (sampleViewZ > sceneDist)
        {
            continue;
        }

        float3 currentPos = gFrameData.cameraWorldPosition + (rayDir * sampleViewZ);

        // --- シャドウ ---
        float4 shadowCoord = mul(float4(currentPos, 1.0f), gFrameData.lightViewProj);
        shadowCoord.xyz /= shadowCoord.w;
        float2 shadowUV = shadowCoord.xy * float2(0.5f, -0.5f) + 0.5f;
        float shadowVisibility = 1.0f; // 範囲外はデフォルトで影
        if (all(shadowUV >= 0.0f) && all(shadowUV <= 1.0f) && shadowCoord.z >= 0.0f && shadowCoord.z <= 1.0f)
        {
            // バイアスを極小に調整（リーク対策）
            shadowVisibility = gShadowMap.SampleCmpLevelZero(gShadowSampler, shadowUV, shadowCoord.z - 0.0001f);
        }

        // --- 流体フェイク ---
        float objRadius = max(gFogSettings.objectRadius, 0.001f);
        float3 vecToPos = currentPos - gFogSettings.objectPos;
        float distToObj = length(vecToPos);
        float3 dirToPos = vecToPos / (distToObj + 0.0001f);
        float speed = length(gFogSettings.objectVelocity);
        float3 velDir = speed > 0.0001f ? (gFogSettings.objectVelocity / speed) : float3(0, 1.0f, 0);

        float pushFactor = smoothstep(objRadius * 2.0f, 0.0f, distToObj);
        float3 pushWarp = dirToPos * (pushFactor * objRadius * 1.5f);
        float3 swirlAxis = normalize(cross(velDir, dirToPos) + float3(0.001f, 0.001f, 0.001f));
        float3 swirlWarp = swirlAxis * (pushFactor * speed * 0.8f);
        float distAlongWake = dot(vecToPos, -velDir);
        float distFromWakeCenter = length(vecToPos - (-velDir * distAlongWake));
        float wakeFactor = smoothstep(max(speed * 3.0f, objRadius * 2.0f), 0.0f, max(distAlongWake, 0.0f)) * smoothstep(objRadius * 1.5f, 0.0f, distFromWakeCenter);
        float3 wakeWarp = velDir * (wakeFactor * speed * gFogSettings.interactionPower);

        float3 noiseSamplePos = currentPos + pushWarp + swirlWarp - wakeWarp;
        float coreMask = lerp(0.8f, 1.0f, smoothstep(objRadius * 0.4f, objRadius * 0.9f, distToObj));

        // --- ノイズサンプリング ---
        float3 timeOffset = float3(1.0f, 0.5f, 0.8f) * (gFrameData.gTime * gFogSettings.windSpeed);
        float3 warpUVW = noiseSamplePos * (gFogSettings.noiseScale * 0.5f) + timeOffset * 0.5f;
        float3 distortion = float3(
            gNoiseVolume.SampleLevel(gSampler, frac(warpUVW), 0).r,
            gNoiseVolume.SampleLevel(gSampler, frac(warpUVW + 0.33f), 0).r,
            gNoiseVolume.SampleLevel(gSampler, frac(warpUVW + 0.67f), 0).r
        );
        
        float3 distortedPos = noiseSamplePos + (distortion * 2.0f - 1.0f) * (gFogSettings.noiseDistortion + wakeFactor * 0.5f);
        float noise1 = gNoiseVolume.SampleLevel(gSampler, frac(distortedPos * gFogSettings.noiseScale + timeOffset), 0).r;
        float noise2 = gNoiseVolume.SampleLevel(gSampler, frac(distortedPos * (gFogSettings.noiseScale * 3.0f) - timeOffset * 0.8f), 0).r;

        float combinedNoise = saturate(noise1 - (1.0f - noise2) * 0.3f);
        float noiseVal = smoothstep(gFogSettings.noiseThreshold, gFogSettings.noiseThreshold + 0.15f, combinedNoise);

        // 1. 密度の計算
        float heightFactor = exp(-max(currentPos.y - gFogSettings.baseHeight, 0.0f) * gFogSettings.heightFalloff);
        float particleDensity = gFogSettings.globalDensity + (gFogSettings.heightDensity * heightFactor * noiseVal * coreMask);

        // 2. 光学プロパティ
        float3 sigma_s = gFogSettings.scatteringColor * particleDensity * gFogSettings.scatteringIntensity;
        float sigma_e = max(particleDensity * gFogSettings.extinctionScale, 0.00001f);

        // 3. 空間に降り注ぐ光
        float cosTheta = dot(rayDir, normalize(-gFrameData.mainLightDirection));
        float phase = DualPhaseHG(cosTheta, gFogSettings.anisotropy);

        // メインライト + 環境光
        float3 incidentLight = shadowVisibility * phase * gFrameData.mainLightColor.rgb;
        incidentLight += gFogSettings.ambientLight * lerp(0.3f, 1.0f, shadowVisibility);

        // ローカルライト
        float3 stepLocal = 0;
        for (int p = 0; p < MAX_POINT_LIGHTS; ++p)
        {
            if (gPointLights[p].enable == 0)
                continue;
            float3 lightVec = gPointLights[p].position - currentPos;
            float distSq = dot(lightVec, lightVec);
            float radiusSq = gPointLights[p].radius * gPointLights[p].radius;
            if (distSq > radiusSq)
                continue;
            
            float dist = sqrt(distSq);
            float attenuation = saturate(1.0f - (distSq / radiusSq));
            attenuation *= attenuation;
            
            float phaseLocal = DualPhaseHG(dot(rayDir, lightVec / dist), 0.0f);
            stepLocal += gPointLights[p].color.rgb * gPointLights[p].intensity * attenuation * phaseLocal;
        }

        for (int s = 0; s < MAX_SPOT_LIGHTS; ++s)
        {
            if (gSpotLights[s].enable == 0)
                continue;
            float3 lightVec = gSpotLights[s].position - currentPos;
            float distSq = dot(lightVec, lightVec);
            float distanceSq = gSpotLights[s].distance * gSpotLights[s].distance;
            if (distSq > distanceSq)
                continue;
            
            float dist = sqrt(distSq);
            float3 lDir = lightVec / dist;
            float currentCos = dot(-lDir, normalize(gSpotLights[s].direction));
            if (currentCos < gSpotLights[s].cosAngle)
                continue;
            
            float angleFalloff = pow(saturate((currentCos - gSpotLights[s].cosAngle) / (1.0f - gSpotLights[s].cosAngle)), 2.0f);
            float distFalloff = saturate(1.0f - (distSq / distanceSq));
            distFalloff *= distFalloff;

            float phaseLocal = DualPhaseHG(dot(rayDir, lDir), 0.0f);
            stepLocal += gSpotLights[s].color.rgb * gSpotLights[s].intensity * angleFalloff * distFalloff * phaseLocal;
        }

        // 光 × 散乱係数
        float3 stepScattering = (incidentLight + stepLocal) * sigma_s;

        accumScattering += stepScattering;
        accumExtinction += sigma_e;
    }

    // --- 平均化と解析的積分 ---
    float3 avgScattering = accumScattering / float(NUM_SAMPLES);
    float avgExtinction = accumExtinction / float(NUM_SAMPLES);

    // ボクセル内の減衰
    float extinctionToPass = avgExtinction * voxelThickness;
    float3 finalScattering = avgScattering * ((1.0f - exp(-extinctionToPass)) / max(avgExtinction, 0.00001f));

    gVoxelInject[DTid.xyz] = float4(finalScattering, extinctionToPass);
}