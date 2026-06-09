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

ConstantBuffer<FogVolumeBuffer> gFogVolumeBuffer : register(b5);

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
    
    // シーンの深度（Depth）を取得し、カメラからの距離を計算
    float hwDepth = gDepthTexture.SampleLevel(gSampler, float2(screenU, screenV), 0).r;
    float4 sceneWorld = mul(float4(clipX, clipY, hwDepth, 1.0f), gFrameData.invViewProj);
    sceneWorld.xyz /= sceneWorld.w;
    float sceneDist = length(sceneWorld.xyz - gFrameData.cameraWorldPosition);

    // ボクセルの手前側が既にオブジェクトの裏側にある場合、まるごと計算をスキップ
    if (viewZ0 > sceneDist)
    {
        gVoxelInject[DTid.xyz] = float4(0, 0, 0, 0);
        return;
    }

    float noiseJitter = InterleavedGradientNoise(float2(DTid.xy), gFrameData.frameIndex);
    
    float3 accumScattering = 0;
    float accumExtinction = 0;
    
    const int NUM_SAMPLES = 2;

    for (int i = 0; i < NUM_SAMPLES; ++i)
    {
        float t = (float(i) + noiseJitter) / float(NUM_SAMPLES);
        float sampleViewZ = viewZ0 + voxelThickness * t;

        float3 currentPos = gFrameData.cameraWorldPosition + (rayDir * sampleViewZ);

        // シャドウの計算
        float4 shadowCoord = mul(float4(currentPos, 1.0f), gFrameData.lightViewProj);
        shadowCoord.xyz /= shadowCoord.w;
        float2 shadowUV = shadowCoord.xy * float2(0.5f, -0.5f) + 0.5f;
        float shadowVisibility = 1.0f;
        if (all(shadowUV >= 0.0f) && all(shadowUV <= 1.0f) && shadowCoord.z >= 0.0f && shadowCoord.z <= 1.0f)
        {
            shadowVisibility = gShadowMap.SampleCmpLevelZero(gShadowSampler, shadowUV, shadowCoord.z - 0.0001f);
        }

        // 流体フェイク ＆ ノイズの計算
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

        // 受光量の計算
        float cosTheta = dot(rayDir, normalize(-gFrameData.mainLightDirection));
        float phase = DualPhaseHG(cosTheta, gFogSettings.anisotropy);
        
        float3 totalLight = shadowVisibility * phase * gFrameData.mainLightColor.rgb;
        totalLight += gFogSettings.ambientLight * lerp(0.3f, 1.0f, shadowVisibility);

        // ローカルライト（ポイント）
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

        // ローカルライト（スポット）
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
        
        totalLight += stepLocal;

        // グローバルフォグの計算
        float heightFactor = exp(-max(currentPos.y - gFogSettings.baseHeight, 0.0f) * gFogSettings.heightFalloff);
        float particleDensity = gFogSettings.globalDensity + (gFogSettings.heightDensity * heightFactor * noiseVal * coreMask);

        float fadeStart = farZ * 0.8f;
        float distanceFade = saturate((farZ - sampleViewZ) / max(farZ - fadeStart, 0.001f));
        particleDensity *= distanceFade;

        float3 global_sigma_s = gFogSettings.scatteringColor * particleDensity * gFogSettings.scatteringIntensity;
        float global_sigma_e = max(particleDensity * gFogSettings.extinctionScale, 0.00001f);

        // 配置式フォグボリュームの計算
        float3 volumeScattering = 0;
        float volumeExtinction = 0;

        for (uint v = 0; v < gFogVolumeBuffer.volumeCount; ++v)
        {
            FogVolume vol = gFogVolumeBuffer.volumes[v];
            float3 localPos = mul(float4(currentPos, 1.0f), vol.worldToLocal).xyz;
            
            float volumeMask = 0.0f;
            if (vol.type == 1) // Box型
            {
                float3 distToEdge = 1.0f - abs(localPos);
                if (all(distToEdge > 0.0f))
                {
                    float3 fade = smoothstep(0.0f, max(vol.blendDistance, 0.001f), distToEdge);
                    volumeMask = fade.x * fade.y * fade.z;
                }
            }
            else // Sphere型
            {
                float dist = length(localPos);
                if (dist < 1.0f)
                {
                    volumeMask = smoothstep(1.0f, 1.0f - vol.blendDistance, dist);
                }
            }

            if (volumeMask > 0.0f)
            {
                float3 volTimeOffset = normalize(vol.windDirection + 0.001f) * (gFrameData.gTime * vol.windSpeed);
                float3 volNoisePos = currentPos * vol.noiseScale + volTimeOffset;
                float rawNoise = gNoiseVolume.SampleLevel(gSampler, frac(volNoisePos), 0).r;
                
                float volNoise = smoothstep(vol.noiseThreshold, vol.noiseThreshold + 0.15f, rawNoise);
                float finalVolDensity = vol.density * lerp(1.0f, volNoise, vol.noiseIntensity) * volumeMask;

                float phaseVol = DualPhaseHG(cosTheta, vol.anisotropy);
                float3 volLight = shadowVisibility * phaseVol * gFrameData.mainLightColor.rgb;
                
                volLight += gFogSettings.ambientLight * lerp(0.3f, 1.0f, shadowVisibility);
                volLight += stepLocal;

                volumeExtinction += finalVolDensity * gFogSettings.extinctionScale;
                volumeScattering += vol.color * finalVolDensity * gFogSettings.scatteringIntensity * volLight;
            }
        }

        // グローバルとボリュームの合算・累積
        float3 sampleScattering = (totalLight * global_sigma_s) + volumeScattering;
        float sampleExtinction = global_sigma_e + volumeExtinction;

        // ソフト・クリッピングの計算
        float fadeRange = max(voxelThickness * 1.0f, 0.1f);
        float depthWeight = saturate((sceneDist - sampleViewZ) / fadeRange);

        // 重みを適用して安全に累積
        accumScattering += sampleScattering * depthWeight;
        accumExtinction += sampleExtinction * depthWeight;
    }

    // 平均化と解析的積分
    float3 avgScattering = accumScattering / float(NUM_SAMPLES);
    float avgExtinction = accumExtinction / float(NUM_SAMPLES);

    float extinctionToPass = avgExtinction * voxelThickness;
    float3 finalScattering = avgScattering * ((1.0f - exp(-extinctionToPass)) / max(avgExtinction, 0.00001f));

    gVoxelInject[DTid.xyz] = float4(finalScattering, extinctionToPass);
}