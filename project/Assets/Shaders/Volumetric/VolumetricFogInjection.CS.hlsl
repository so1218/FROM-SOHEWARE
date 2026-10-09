#include "Common/ShaderConstants.hlsli"
#include "Common/Object3D.hlsli"
#include "Common/CameraUtils.hlsli"
#include "Common/MathUtils.hlsli"

Texture2D<float> gDepthTexture : register(t0);
Texture2DArray<float> gShadowMap : register(t1);
Texture3D<float4> gNoiseVolume : register(t2);
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
ConstantBuffer<ShadowData> gShadowData : register(b6);
ConstantBuffer<GlobalEnvironmentData> gEnvironmentData : register(b7);

// Henyey-Greenstein 位相関数
float PhaseFunctionHG(float cosTheta, float g)
{
    float g2 = g * g;
    float denom = 1.0f + g2 - 2.0f * g * cosTheta;
    return (1.0f - g2) / (4.0f * PI * pow(max(denom, 0.0001f), 1.5f));
}

// 二重 Henyey-Greenstein 位相関数
float DualPhaseHG(float cosTheta, float gForward)
{
    static const float kBackScatterG = -0.2f;
    static const float kBlendRatio = 0.9f;

    float forward = PhaseFunctionHG(cosTheta, gForward);
    float backward = PhaseFunctionHG(cosTheta, kBackScatterG);
    return lerp(backward, forward, kBlendRatio);
}

// レイマーチングのアーティファクトを消すためのノイズ関数
float InterleavedGradientNoise(float2 pixelCoord, uint frameIndex)
{
    static const float kTemporalGoldenRatio = 5.588238f;
    pixelCoord += float2(frameIndex * kTemporalGoldenRatio, frameIndex * kTemporalGoldenRatio);
    
    static const float3 kIGNMagic = float3(0.06711056f, 0.00583715f, 52.9829189f);
    return frac(kIGNMagic.z * frac(dot(pixelCoord, kIGNMagic.xy)));
}

static const float3 kNoiseOffsetA = float3(131.0f, 73.0f, 191.0f);
static const float3 kNoiseOffsetB = float3(43.0f, 269.0f, 113.0f);
static const float3 kNoiseOffsetC = float3(411.0f, 823.0f, 157.0f);
static const float kCameraNearFadeStart = 0.5f;
static const float kCameraNearFadeEnd = 3.0f;
static const float kMinSafeDistance = 0.001f;

[numthreads(8, 8, 4)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    // ---------------------------------------------------------
    // Froxel空間の復元とサンプリング用ジッター
    // ---------------------------------------------------------
    uint width, height, depth;
    gVoxelInject.GetDimensions(width, height, depth);
    if (DTid.x >= width || DTid.y >= height || DTid.z >= depth)
        return;

    float nearZ = max(gFrameData.nearClip, kMinNearClip);
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

    float hwDepth = gDepthTexture.SampleLevel(gSampler, float2(screenU, screenV), 0).r;
    float4 sceneWorld = mul(float4(clipX, clipY, hwDepth, 1.0f), gFrameData.invViewProj);
    sceneWorld.xyz /= sceneWorld.w;
    float sceneDist = length(sceneWorld.xyz - gFrameData.cameraWorldPosition);

    float noiseJitter = InterleavedGradientNoise(DTid.xy, gFrameData.frameIndex);
    float sampleViewZ = viewZ0 + voxelThickness * noiseJitter;

    // ---------------------------------------------------------
    // ジオメトリカリングと境界のソフト化
    // ---------------------------------------------------------
    float fadeRange = max(voxelThickness * 1.0f, 0.1f);
    
    if (sampleViewZ > sceneDist + fadeRange)
    {
        gVoxelInject[DTid.xyz] = float4(0.0f, 0.0f, 0.0f, 0.0f);
        return;
    }

    float depthWeight = saturate((sceneDist - sampleViewZ) / fadeRange);
    float nearFade = smoothstep(kCameraNearFadeStart, kCameraNearFadeEnd, sampleViewZ);
    depthWeight *= nearFade;

    float3 currentPos = gFrameData.cameraWorldPosition + (rayDir * sampleViewZ);

    // ---------------------------------------------------------
    // カスケードシャドウ
    // ---------------------------------------------------------
    uint cascadeIndex = 0;
    if (sampleViewZ > gShadowData.cascadeSplits[1])
        cascadeIndex = 2;
    else if (sampleViewZ > gShadowData.cascadeSplits[0])
        cascadeIndex = 1;

    float4 shadowCoord = mul(float4(currentPos, 1.0f), gShadowData.cascadeLightViewProj[cascadeIndex]);
    shadowCoord.xyz /= shadowCoord.w;
    float2 shadowUV = shadowCoord.xy * float2(0.5f, -0.5f) + 0.5f;

    float shadowVisibility = 1.0f;
    if (all(shadowUV >= 0.0f) && all(shadowUV <= 1.0f) && shadowCoord.z >= 0.0f && shadowCoord.z <= 1.0f)
    {
        shadowVisibility = gShadowMap.SampleCmpLevelZero(
            gShadowSampler,
            float3(shadowUV, cascadeIndex),
            shadowCoord.z - kMinSafeDistance
        );
    }

    // ---------------------------------------------------------
    // ボリューメトリックノイズの合成
    // ---------------------------------------------------------
    // CPUで連続積算された 2D オフセットを XZ 平面へ展開
    float3 windOffset3D = float3(
        gEnvironmentData.windOffset.x,
        0.0f,
        gEnvironmentData.windOffset.y
    ) * gFogSettings.windSpeedMultiplier;

    // 歪みサンプリング
    float3 warpUVW = currentPos * (gFogSettings.noiseScale * 0.43f) - windOffset3D * 0.35f;
    float3 distortion = float3(
        gNoiseVolume.SampleLevel(gSampler, warpUVW, 0).r,
        gNoiseVolume.SampleLevel(gSampler, warpUVW + kNoiseOffsetA, 0).r,
        gNoiseVolume.SampleLevel(gSampler, warpUVW + kNoiseOffsetB, 0).r
    );

    float3 distortedPos = currentPos + (distortion * 2.0f - 1.0f) * gFogSettings.noiseDistortion;

    // マイナスで減算することで、風下へノイズが流れる
    float3 uvwA = distortedPos * gFogSettings.noiseScale - windOffset3D;
    float4 noiseLayer1 = gNoiseVolume.SampleLevel(gSampler, uvwA, 0);

    float3 uvwB = distortedPos * (gFogSettings.noiseScale * 0.37f) - (windOffset3D * 1.41f) + kNoiseOffsetC;
    float4 noiseLayer2 = gNoiseVolume.SampleLevel(gSampler, uvwB, 0);

    float combinedPerlin = noiseLayer1.r * noiseLayer2.r * 1.5f;

    float linearDistanceRatio = saturate(sampleViewZ / farZ);
    float detailFade = smoothstep(0.1f, 0.6f, linearDistanceRatio);

    float activeWorleyWeight = lerp(gFogSettings.worleyWeight, 0.0f, detailFade);
    float activeErosion = lerp(gFogSettings.erosion, 0.0f, detailFade);
    float activeNoiseIntensity = lerp(gFogSettings.noiseIntensity, gFogSettings.noiseIntensity * 0.2f, detailFade);

    float combinedNoise = lerp(combinedPerlin, 1.0f - noiseLayer1.g, activeWorleyWeight);
    combinedNoise = saturate(combinedNoise - (noiseLayer1.b * activeErosion));

    float heightFactor = 1.0f;
    if (gFogSettings.heightFalloff > 0.0f)
    {
        float heightDiff = max(currentPos.y - gFogSettings.baseHeight, 0.0f);
        heightFactor = exp(-heightDiff * gFogSettings.heightFalloff);
    }

    float globalBaseDensity = gFogSettings.extinction + (gFogSettings.heightDensity * heightFactor);

    float cutoff = 1.0f - gFogSettings.coverage;
    float noiseCoverage = smoothstep(cutoff, cutoff + max(gFogSettings.noiseFeather, kMinSafeDistance), combinedNoise);
    float erosionFactor = saturate(1.0f - (1.0f - combinedNoise) * gFogSettings.erosionStrength);

    float combinedNoiseEffect = erosionFactor * noiseCoverage;
    float finalNoiseModifier = lerp(1.0f, combinedNoiseEffect, activeNoiseIntensity);

    float particleDensity = globalBaseDensity * finalNoiseModifier;

    // ---------------------------------------------------------
    // ライティングと光の減衰
    // ---------------------------------------------------------
    float dynamicShadowFog = globalBaseDensity * combinedNoiseEffect;
    float fogSelfShadow = exp(-dynamicShadowFog * 4.0f);
    float finalShadowVisibility = shadowVisibility * fogSelfShadow;

    float cosTheta = dot(rayDir, normalize(-gFrameData.mainLightDirection));
    float phase = DualPhaseHG(cosTheta, gFogSettings.anisotropy);

    float3 mainLightContrib = finalShadowVisibility * phase * gFrameData.mainLightColor.rgb * gFrameData.mainLightVolumetricScatteringIntensity;
    float3 ambientContrib = gFogSettings.ambientLight * lerp(0.3f, 1.0f, finalShadowVisibility);
    float3 totalLight = mainLightContrib + ambientContrib;

    float3 stepLocal = 0;

    for (int p = 0; p < MAX_POINT_LIGHTS; ++p)
    {
        if (gPointLights[p].enable == 0)
            continue;

        float3 lightVec = gPointLights[p].position - currentPos;
        float distance = length(lightVec);
        float radius = gPointLights[p].radius;

        float smoothDistance = sqrt(distance * distance + voxelThickness * voxelThickness * 0.25f);
        if (smoothDistance > radius * 1.1f)
            continue;

        float3 lightDir = lightVec / max(distance, kMinSafeDistance);

        float baseAttenuate = pow(saturate(1.0f - smoothDistance / radius), 2.0f);
        float edgeFadeOut = smoothstep(radius * 1.1f, radius * 0.95f, smoothDistance);
        
        float phaseLocal = DualPhaseHG(dot(rayDir, lightDir), gFogSettings.anisotropy);
        float pointLocalFogAttenuation = exp(-particleDensity * 1.0f);

        stepLocal += gPointLights[p].color.rgb * (gPointLights[p].intensity * gPointLights[p].volumetricScatteringIntensity)
                   * (baseAttenuate * edgeFadeOut) * phaseLocal * pointLocalFogAttenuation;
    }

    for (int s = 0; s < MAX_SPOT_LIGHTS; ++s)
    {
        if (gSpotLights[s].enable == 0)
            continue;

        float3 lightVec = gSpotLights[s].position - currentPos;
        float distance = length(lightVec);
        float smoothDistance = sqrt(distance * distance + voxelThickness * voxelThickness * 0.25f);

        if (smoothDistance > gSpotLights[s].distance * 1.2f)
            continue;

        float3 lDir = lightVec / max(distance, kMinSafeDistance);
        
        if (distance < 0.2f)
        {
            float blend = smoothstep(0.0f, 0.2f, distance);
            lDir = normalize(lerp(-gSpotLights[s].direction, lDir, blend));
        }

        float currentCos = dot(-lDir, normalize(gSpotLights[s].direction));
        float cosOuter = gSpotLights[s].cosAngle;
        float voxelSmoothing = (voxelThickness / max(distance, 1.0f)) * 0.15f;

        if (currentCos < cosOuter - voxelSmoothing)
            continue;

        float safeMaxDist = max(gSpotLights[s].distance, kMinSafeDistance);
        float distanceAtt = pow(saturate(1.0f - saturate(smoothDistance / safeMaxDist)), 2.0f);

        float cosInner = min(cosOuter + 0.02f + voxelSmoothing, 1.0f);
        float rawAngleAtt = saturate((currentCos - (cosOuter - voxelSmoothing)) / max(cosInner - (cosOuter - voxelSmoothing), kMinSafeDistance));
        
        float attenuation = distanceAtt * (rawAngleAtt * rawAngleAtt);
        if (attenuation <= 0.0f)
            continue;

        float phaseSmoothing = saturate(1.0f - (voxelThickness / max(distance, 0.5f)));
        float phaseLocal = DualPhaseHG(dot(rayDir, lDir), gFogSettings.anisotropy * phaseSmoothing);
        float spotLocalFogAttenuation = exp(-particleDensity * 1.0f);

        stepLocal += gSpotLights[s].color.rgb * (gSpotLights[s].intensity * gSpotLights[s].volumetricScatteringIntensity)
                   * attenuation * phaseLocal * spotLocalFogAttenuation;
    }

    totalLight += stepLocal;

    // ---------------------------------------------------------
    // ローカルフォグボリュームの評価
    // ---------------------------------------------------------
    float fadeStart = farZ * 0.8f;
    float distanceFade = saturate((farZ - sampleViewZ) / max(farZ - fadeStart, kMinSafeDistance));
    particleDensity *= distanceFade;

    float global_sigma_e = max(particleDensity * gFogSettings.extinctionScale, kEpsilon);
    float3 global_sigma_s = global_sigma_e * gFogSettings.albedo;

    float3 volumeScattering = 0;
    float volumeExtinction = 0;

    for (uint v = 0; v < gFogVolumeBuffer.volumeCount; ++v)
    {
        FogVolume vol = gFogVolumeBuffer.volumes[v];
        
        float3 localPos = mul(float4(currentPos, 1.0f), vol.worldToLocal).xyz;

        float volumeMask = 0.0f;
        if (vol.type == 1) // Box
        {
            float3 distToEdge = 1.0f - abs(localPos);
            if (all(distToEdge > 0.0f))
            {
                float3 fade = smoothstep(0.0f, max(vol.blendDistance, kMinSafeDistance), distToEdge);
                volumeMask = fade.x * fade.y * fade.z;
            }
        }
        else // Sphere
        {
            float dist = length(localPos);
            if (dist < 1.0f)
                volumeMask = smoothstep(1.0f, 1.0f - vol.blendDistance, dist);
        }

        if (volumeMask > 0.0f)
        {
            // 2D オフセットベース
            float3 volWindOffset3D = float3(
            gEnvironmentData.windOffset.x + vol.windDirection.x * gEnvironmentData.windTime, 
            vol.windDirection.y * gEnvironmentData.windTime,
            gEnvironmentData.windOffset.y + vol.windDirection.z * gEnvironmentData.windTime) * vol.windSpeed;

            // マイナス減算で風下へ流す
            float3 volWarpUVW = (currentPos * vol.noiseScale * 0.4f) - volWindOffset3D * 0.5f;
            float3 volWarp = gNoiseVolume.SampleLevel(gSampler, volWarpUVW, 0.0f).rgb * 2.0f - 1.0f;
            float3 volNoisePos = (currentPos * vol.noiseScale) - volWindOffset3D + (volWarp * vol.distortionAmount);

            float4 volNoiseSample = gNoiseVolume.SampleLevel(gSampler, volNoisePos, 0.0f);

            float vNoise = lerp(volNoiseSample.r, 1.0f - volNoiseSample.g, vol.worleyWeight);
            vNoise = saturate(vNoise - (1.0f - vNoise) * volNoiseSample.b * vol.erosion);

            float volCutoff = 1.0f - (vol.coverage * volumeMask);
            float shiftedNoise = vNoise + vol.densityOffset;
            float volCoverage = smoothstep(volCutoff, volCutoff + max(vol.noiseFeather, kMinSafeDistance), shiftedNoise);

            float erosionFactorVol = saturate(1.0f - (1.0f - shiftedNoise) * max(vol.noiseContrast, 1.0f));
            float noiseModifierVol = lerp(1.0f, erosionFactorVol * volCoverage, vol.noiseIntensity);

            float localUVW_Y = localPos.y * 0.5f + 0.5f;
            float finalVolDensity = vol.density * exp(-localUVW_Y * max(vol.heightFalloff, 0.0f)) * noiseModifierVol * volumeMask;

            float phaseVol = DualPhaseHG(0.0f, vol.anisotropy);
            
            float3 volLight = phaseVol * gFrameData.mainLightColor.rgb;
            volLight += gFogSettings.ambientLight;

            volumeExtinction += finalVolDensity * gFogSettings.extinctionScale;
            volumeScattering += vol.color * finalVolDensity * gFogSettings.scatteringIntensity * volLight;
        }
    }

    // ---------------------------------------------------------
    // ボクセルへの書き込み
    // ---------------------------------------------------------
    float3 scattering = ((totalLight * global_sigma_s) + volumeScattering) * depthWeight;
    float extinction = (global_sigma_e + volumeExtinction) * depthWeight;

    gVoxelInject[DTid.xyz] = float4(scattering, extinction);
}