#include "Common/ShaderConstants.hlsli"
#include "Common/MathUtils.hlsli"
#include "Volumetric/FroxelCommon.hlsli"

Texture2D<float> gDepthTexture : register(t0);
Texture2DArray<float> gShadowMap : register(t1);
Texture3D<float4> gNoiseVolume : register(t2);
SamplerState gSampler : register(s0);
SamplerComparisonState gShadowSampler : register(s1);

// rgb: 散乱した光, a: 減衰係数
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

// カメラのすぐ前の霧は消す
static const float kCameraNearFadeStart = 0.5f;
static const float kCameraNearFadeEnd = 3.0f;
static const float kMinSafeDistance = 0.001f;

// 太陽を背にしても真っ暗にならないよう、後方散乱を少し混ぜる
static const float kBackScatterAnisotropy = -0.2f;
static const float kForwardScatterRatio = 0.9f;

// 自己影
static const uint kSelfShadowStepCount = 4;
static const float kSelfShadowFirstStep = 1.0f;

struct FogMedia
{
    float density;     
    float3 scattering;
};

float PhaseHenyeyGreenstein(float cosTheta, float anisotropy)
{
    float anisotropySquared = anisotropy * anisotropy;
    float denominator = 1.0f + anisotropySquared - 2.0f * anisotropy * cosTheta;
    return (1.0f - anisotropySquared) / (4.0f * PI * pow(max(denominator, 0.0001f), 1.5f));
}

float PhaseDualHenyeyGreenstein(float cosTheta, float forwardAnisotropy)
{
    float forward = PhaseHenyeyGreenstein(cosTheta, forwardAnisotropy);
    float backward = PhaseHenyeyGreenstein(cosTheta, kBackScatterAnisotropy);
    return lerp(backward, forward, kForwardScatterRatio);
}

// 毎フレーム模様をずらした IGN
float TemporalInterleavedGradientNoise(float2 pixelCoord, uint frameIndex)
{
    pixelCoord += frameIndex * 5.588238f;
    return frac(52.9829189f * frac(dot(pixelCoord, float2(0.06711056f, 0.00583715f))));
}

// 霧のむら。全体の霧と置いた霧で同じ式
float CalculateNoiseShape(float4 noise, float coverage, float softness, float erosion)
{
    // パーリンをウォーリーで持ち上げて塊に
    float shape = saturate((noise.r - (noise.g - 1.0f)) / max(2.0f - noise.g, kEpsilon));

    // 細かいウォーリーで縁を削る
    float erosionAmount = noise.b * erosion;
    shape = saturate((shape - erosionAmount) / max(1.0f - erosionAmount, kEpsilon));

    // coverage が霧のある範囲、softness が境目のぼかし
    float coverageStart = 1.0f - coverage;
    return smoothstep(coverageStart, coverageStart + max(softness, kMinSafeDistance), shape) * shape;
}

// 置いた霧の内側なら 1、外側なら 0。端は blendDistance でぼかす
float CalculateVolumeMask(FogVolume volume, float3 localPosition)
{
    if (volume.type == 1) // 箱
    {
        float3 distanceToEdge = 1.0f - abs(localPosition);
        if (any(distanceToEdge <= 0.0f))
            return 0.0f;
        float3 fade = smoothstep(0.0f, max(volume.blendDistance, kMinSafeDistance), distanceToEdge);
        return fade.x * fade.y * fade.z;
    }

    // 球
    float distanceFromCenter = length(localPosition);
    if (distanceFromCenter >= 1.0f)
        return 0.0f;
    return smoothstep(1.0f, 1.0f - volume.blendDistance, distanceFromCenter);
}

// detailRatio が 0 に近いほど細かい模様を省く
FogMedia EvaluateFogMedia(float3 position, float detailRatio, bool isDetailed)
{
    FogMedia media;
    media.density = 0.0f;
    media.scattering = float3(0.0f, 0.0f, 0.0f);

    // 全体の霧は、一様な濃さに地面近くの濃さを足したもの
    float heightFactor = 1.0f;
    if (gFogSettings.heightFalloff > 0.0f)
    {
        heightFactor = exp(-max(position.y - gFogSettings.baseHeight, 0.0f) * gFogSettings.heightFalloff);
    }
    float globalDensity = gFogSettings.extinction + gFogSettings.heightDensity * heightFactor;

    if (gFogSettings.noiseIntensity > 0.0f)
    {
        float3 windOffset = float3(gEnvironmentData.windOffset.x, 0.0f, gEnvironmentData.windOffset.y) * gFogSettings.windSpeedMultiplier;
        float3 noisePosition = position;
        if (isDetailed)
        {
            float3 warp = gNoiseVolume.SampleLevel(gSampler, position * (gFogSettings.noiseScale * 0.43f) - windOffset * 0.35f, 0).rgb;
            noisePosition += (warp * 2.0f - 1.0f) * gFogSettings.noiseDistortion;
        }
        float4 noise = gNoiseVolume.SampleLevel(gSampler, noisePosition * gFogSettings.noiseScale - windOffset, 0);
        float shape = CalculateNoiseShape(noise, gFogSettings.coverage, gFogSettings.noiseFeather, gFogSettings.erosion * detailRatio);

        // 遠くではむらを弱める
        float noiseIntensity = gFogSettings.noiseIntensity * lerp(0.2f, 1.0f, detailRatio);
        globalDensity *= lerp(1.0f, shape, noiseIntensity);
    }

    media.density += globalDensity;
    media.scattering += globalDensity * gFogSettings.albedo;

    for (uint volumeIndex = 0; volumeIndex < gFogVolumeBuffer.volumeCount; ++volumeIndex)
    {
        FogVolume volume = gFogVolumeBuffer.volumes[volumeIndex];
        float3 localPosition = mul(float4(position, 1.0f), volume.worldToLocal).xyz;
        float volumeMask = CalculateVolumeMask(volume, localPosition);
        if (volumeMask <= 0.0f)
            continue;

        float3 volumeWind = float3(
            gEnvironmentData.windOffset.x + volume.windDirection.x * gEnvironmentData.windTime,
            volume.windDirection.y * gEnvironmentData.windTime,
            gEnvironmentData.windOffset.y + volume.windDirection.z * gEnvironmentData.windTime) * volume.windSpeed;

        float3 noisePosition = position * volume.noiseScale - volumeWind;
        if (isDetailed)
        {
            float3 warp = gNoiseVolume.SampleLevel(gSampler, position * volume.noiseScale * 0.4f - volumeWind * 0.5f, 0).rgb;
            noisePosition += (warp * 2.0f - 1.0f) * volume.distortionAmount;
        }
        float4 noise = gNoiseVolume.SampleLevel(gSampler, noisePosition, 0);

        // 端ほど coverage を下げて、輪郭のある塊にする
        float shape = CalculateNoiseShape(noise, volume.coverage * volumeMask, volume.noiseFeather, volume.erosion * detailRatio);

        float heightInVolume = localPosition.y * 0.5f + 0.5f;
        float volumeDensity = volume.density * volumeMask
                            * exp(-heightInVolume * max(volume.heightFalloff, 0.0f))
                            * lerp(1.0f, shape, volume.noiseIntensity);

        media.density += volumeDensity;
        media.scattering += volumeDensity * volume.color;
    }

    return media;
}

float SampleSunShadow(float3 worldPosition, float distanceFromCamera)
{
    uint cascadeIndex = 0;
    [unroll]
    for (uint i = 0; i < MAX_CASCADE_COUNT - 1; ++i)
    {
        if (distanceFromCamera > gShadowData.cascadeSplits[i])
            cascadeIndex = i + 1;
    }

    float4 shadowClip = mul(float4(worldPosition, 1.0f), gShadowData.cascadeLightViewProj[cascadeIndex]);
    float3 shadowPosition = shadowClip.xyz / shadowClip.w;
    float2 shadowUV = shadowPosition.xy * float2(0.5f, -0.5f) + 0.5f;

    if (any(shadowUV < 0.0f) || any(shadowUV > 1.0f) || shadowPosition.z < 0.0f || shadowPosition.z > 1.0f)
        return 1.0f;

    return gShadowMap.SampleCmpLevelZero(gShadowSampler, float3(shadowUV, cascadeIndex), shadowPosition.z - kMinSafeDistance);
}

// 太陽までの霧で、光がどれだけ残るか
float CalculateFogSelfShadow(float3 position, float3 toSun, float detailRatio)
{
    float opticalDepth = 0.0f;
    float stepStart = 0.0f;
    float stepLength = kSelfShadowFirstStep;
    for (uint i = 0; i < kSelfShadowStepCount; ++i)
    {
        float3 samplePosition = position + toSun * (stepStart + stepLength * 0.5f);
        opticalDepth += EvaluateFogMedia(samplePosition, detailRatio, false).density * stepLength;
        stepStart += stepLength;
        stepLength *= 2.0f;
    }
    return exp(-opticalDepth * gFogSettings.extinctionScale);
}

[numthreads(8, 8, 4)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    uint froxelWidth, froxelHeight, froxelDepth;
    gVoxelInject.GetDimensions(froxelWidth, froxelHeight, froxelDepth);
    if (any(dispatchThreadId >= uint3(froxelWidth, froxelHeight, froxelDepth)))
        return;

    float nearDistance = GetFroxelNearDistance(gFrameData.nearClip);
    float farDistance = GetFroxelFarDistance(gFrameData.farClip, gFogSettings.maxDistance);

    float sliceStart = FroxelSliceToDistance(dispatchThreadId.z, froxelDepth, nearDistance, farDistance);
    float sliceEnd = FroxelSliceToDistance(dispatchThreadId.z + 1.0f, froxelDepth, nearDistance, farDistance);
    float sliceThickness = sliceEnd - sliceStart;

    float2 screenUV = (float2(dispatchThreadId.xy) + 0.5f) / float2(froxelWidth, froxelHeight);
    float3 rayDirection = GetViewRayDirection(screenUV, gFrameData.invViewProj, gFrameData.cameraWorldPosition);

    float sceneDepth = gDepthTexture.SampleLevel(gSampler, screenUV, 0).r;
    float4 sceneWorld = mul(float4(screenUV.x * 2.0f - 1.0f, (1.0f - screenUV.y) * 2.0f - 1.0f, sceneDepth, 1.0f), gFrameData.invViewProj);
    float sceneDistance = length(sceneWorld.xyz / sceneWorld.w - gFrameData.cameraWorldPosition);

    // スライス内の1点だけ調べる。位置は毎フレーム変えて、TemporalResolve で平均する
    float sampleDistance = sliceStart + sliceThickness * TemporalInterleavedGradientNoise(dispatchThreadId.xy, gFrameData.frameIndex);

    // 物より奥には霧を置かない。境目はスライス1枚分でぼかす
    float edgeFadeRange = max(sliceThickness, 0.1f);
    if (sampleDistance > sceneDistance + edgeFadeRange)
    {
        gVoxelInject[dispatchThreadId] = float4(0.0f, 0.0f, 0.0f, 0.0f);
        return;
    }

    float visibleWeight = saturate((sceneDistance - sampleDistance) / edgeFadeRange);
    visibleWeight *= smoothstep(kCameraNearFadeStart, kCameraNearFadeEnd, sampleDistance);

    // 最大距離で霧がぷつりと切れないよう、最後の 2 割で薄くする
    float fadeStart = farDistance * 0.8f;
    visibleWeight *= saturate((farDistance - sampleDistance) / max(farDistance - fadeStart, kMinSafeDistance));

    float3 samplePosition = gFrameData.cameraWorldPosition + rayDirection * sampleDistance;

    // 遠くは細かい模様が見えずちらつくだけなので弱める
    float detailRatio = 1.0f - smoothstep(0.1f, 0.6f, saturate(sampleDistance / farDistance));

    FogMedia media = EvaluateFogMedia(samplePosition, detailRatio, true);
    if (media.density <= 0.0f)
    {
        gVoxelInject[dispatchThreadId] = float4(0.0f, 0.0f, 0.0f, kEpsilon);
        return;
    }

    // 完全に影なら自己影を掛けても 0 なので、自己影の計算を飛ばす
    float3 toSun = normalize(-gFrameData.mainLightDirection);
    float sunVisibility = SampleSunShadow(samplePosition, sampleDistance);
    if (sunVisibility > 0.0f)
    {
        sunVisibility *= CalculateFogSelfShadow(samplePosition, toSun, detailRatio);
    }
    float sunPhase = PhaseDualHenyeyGreenstein(dot(rayDirection, toSun), gFogSettings.anisotropy);
    float3 light = gFrameData.mainLightColor.rgb * gFrameData.mainLightVolumetricScatteringIntensity * sunPhase * sunVisibility;

    // 空からの光は向きを考えずに足す
    light += gFogSettings.ambientLight;

    // フロクセルの厚みの分だけ距離をならして、光の境目のガタつきを抑える
    float sliceThicknessSquaredHalf = sliceThickness * sliceThickness * 0.25f;

    for (int pointIndex = 0; pointIndex < MAX_POINT_LIGHTS; ++pointIndex)
    {
        PointLight pointLight = gPointLights[pointIndex];
        if (pointLight.enable == 0)
            continue;

        float3 toLight = pointLight.position - samplePosition;
        float distanceToLight = length(toLight);
        float smoothDistance = sqrt(distanceToLight * distanceToLight + sliceThicknessSquaredHalf);
        if (smoothDistance > pointLight.radius * 1.1f)
            continue;

        float3 lightDirection = toLight / max(distanceToLight, kMinSafeDistance);
        float distanceAttenuation = pow(saturate(1.0f - smoothDistance / pointLight.radius), 2.0f);
        distanceAttenuation *= smoothstep(pointLight.radius * 1.1f, pointLight.radius * 0.95f, smoothDistance);

        float phase = PhaseDualHenyeyGreenstein(dot(rayDirection, lightDirection), gFogSettings.anisotropy);
        light += pointLight.color.rgb * (pointLight.intensity * pointLight.volumetricScatteringIntensity) * distanceAttenuation * phase;
    }

    for (int spotIndex = 0; spotIndex < MAX_SPOT_LIGHTS; ++spotIndex)
    {
        SpotLight spotLight = gSpotLights[spotIndex];
        if (spotLight.enable == 0)
            continue;

        float3 toLight = spotLight.position - samplePosition;
        float distanceToLight = length(toLight);
        float smoothDistance = sqrt(distanceToLight * distanceToLight + sliceThicknessSquaredHalf);
        if (smoothDistance > spotLight.distance * 1.2f)
            continue;

        // ライトのすぐ近くは向きが暴れるので、ライトの向きに寄せる
        float3 lightDirection = toLight / max(distanceToLight, kMinSafeDistance);
        if (distanceToLight < 0.2f)
        {
            lightDirection = normalize(lerp(-spotLight.direction, lightDirection, smoothstep(0.0f, 0.2f, distanceToLight)));
        }

        float cosToSample = dot(-lightDirection, normalize(spotLight.direction));
        float edgeSoftness = (sliceThickness / max(distanceToLight, 1.0f)) * 0.15f;
        float cosOuter = spotLight.cosAngle - edgeSoftness;
        if (cosToSample < cosOuter)
            continue;

        float distanceAttenuation = pow(saturate(1.0f - saturate(smoothDistance / max(spotLight.distance, kMinSafeDistance))), 2.0f);
        float cosInner = min(spotLight.cosAngle + 0.02f + edgeSoftness, 1.0f);
        float angleAttenuation = saturate((cosToSample - cosOuter) / max(cosInner - cosOuter, kMinSafeDistance));
        float attenuation = distanceAttenuation * angleAttenuation * angleAttenuation;
        if (attenuation <= 0.0f)
            continue;

        // 厚いフロクセルでは前方散乱を弱めて、光の筋がちらつかないようにする
        float anisotropyScale = saturate(1.0f - sliceThickness / max(distanceToLight, 0.5f));
        float phase = PhaseDualHenyeyGreenstein(dot(rayDirection, lightDirection), gFogSettings.anisotropy * anisotropyScale);
        light += spotLight.color.rgb * (spotLight.intensity * spotLight.volumetricScatteringIntensity) * attenuation * phase;
    }

    // 散乱係数は減衰係数に散乱色を掛けたもの
    float extinction = max(media.density * gFogSettings.extinctionScale, kEpsilon);
    float3 scattering = media.scattering * gFogSettings.extinctionScale * light;

    gVoxelInject[dispatchThreadId] = float4(scattering, extinction) * visibleWeight;
}
