#include "ShaderConstants.hlsli"
#include "Object3D.hlsli"

Texture2D<float> gDepthTexture : register(t0);
Texture2DArray<float> gShadowMap : register(t1);
Texture3D<float4> gNoiseVolume : register(t2);
Texture3D<float> gFluidDensity : register(t3);
Texture3D<float4> gFluidVelocity : register(t4);
Texture3D<float4> gFluidUVW : register(t5);
SamplerState gSampler : register(s0);
SamplerComparisonState gShadowSampler : register(s1);

// 3Dテクスチャへの出力
RWTexture3D<float4> gVoxelInject : register(u0);

ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<FluidSettings> gFluidSettings : register(b1);
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

// Henyey-Greenstein 位相関数
// 光の非対称な散乱確率を計算する標準モデル
float PhaseFunctionHG(float cosTheta, float g)
{
    float g2 = g * g;
    float denom = 1.0f + g2 - 2.0f * g * cosTheta;
    return (1.0f - g2) / (4.0f * PI * pow(max(denom, 0.0001f), 1.5f));
}

// 二重 Henyey-Greenstein 位相関数
// 実際の霧で発生する強い前方散乱と弱い後方散乱を近似するための標準アプローチ
float DualPhaseHG(float cosTheta, float gForward)
{
    // 一般的な大気・雲の散乱近似パラメータ
    static const float kBackScatterG = -0.2f; // 後方散乱の非対称性
    static const float kBlendRatio = 0.9f; // 前方散乱の優先度 (90% 前方, 10% 後方)

    float forward = PhaseFunctionHG(cosTheta, gForward);
    float backward = PhaseFunctionHG(cosTheta, kBackScatterG);
    return lerp(backward, forward, kBlendRatio);
}

// レイマーチングのアーティファクトを消すためのノイズ関数
// IGN論文の公式
float InterleavedGradientNoise(float2 pixelCoord, uint frameIndex)
{
    // TAA や時間軸のジッターに対応させるため、フレーム単位でオフセット
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

    // 指数関数的なZスライスにより、手前側の解像度を高めに確保
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

    // TAAでの時間的蓄積を前提とし、IGN(Interleaved Gradient Noise)で深度方向にレイをずらす
    float noiseJitter = InterleavedGradientNoise(DTid.xy, gFrameData.frameIndex);
    float sampleViewZ = viewZ0 + voxelThickness * noiseJitter;

    // ---------------------------------------------------------
    // ジオメトリカリングと境界のソフト化
    // ---------------------------------------------------------
    float fadeRange = max(voxelThickness * 1.0f, 0.1f);
    
    // 深度バッファを参照し、不透明ジオメトリの裏側に完全に隠れるボクセルは処理を打ち切る
    if (sampleViewZ > sceneDist + fadeRange)
    {
        gVoxelInject[DTid.xyz] = float4(0.0f, 0.0f, 0.0f, 0.0f);
        return;
    }

    // 地形との交差部でハードエッジが出ないよう、fadeRange区間で徐々にウェイトを落とす
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
    // 流体ボリュームの移流と境界処理
    // ---------------------------------------------------------
    float3 fluidSize = gFluidSettings.gridMax - gFluidSettings.gridMin;
    bool isInsideFluidGrid = all(currentPos >= gFluidSettings.gridMin) && all(currentPos <= gFluidSettings.gridMax);

    float fluidMass = 0.0f;
    float3 advectedFluidPos = currentPos;
    float fluidShadowMass = 0.0f;
    float edgeFade = 0.0f;

    if (isInsideFluidGrid)
    {
        // 領域外で密度がパキッと途切れないよう、境界の10%でフェードアウト
        float3 distToMin = currentPos - gFluidSettings.gridMin;
        float3 distToMax = gFluidSettings.gridMax - currentPos;
        float3 minDist = min(distToMin, distToMax);
        edgeFade = smoothstep(0.0f, fluidSize.x * 0.1f, min(min(minDist.x, minDist.y), minDist.z));

        // Toroidal Wrap境界を前提としたサンプリング
        float3 fluidUVW = frac(currentPos / fluidSize);
        fluidMass = max(gFluidDensity.SampleLevel(gSampler, fluidUVW, 0).r, 0.0f) * edgeFade;

        // 移流ベクトルから歪んだワールド座標を算出（最短経路補正を含む）
        float3 advectedUVW = gFluidUVW.SampleLevel(gSampler, fluidUVW, 0).xyz;
        float3 uvwOffset = advectedUVW - fluidUVW;
        uvwOffset = uvwOffset - floor(uvwOffset + 0.5f);
        advectedFluidPos = currentPos + (uvwOffset * fluidSize);

        // ボリューム自身のセルフシャドウ近似用に、光源方向へ少しオフセットして密度を再評価
        float3 shadowOffsetWorld = normalize(-gFrameData.mainLightDirection) * (2.0f * gFluidSettings.gridScale);
        float3 shadowSamplePos = currentPos + shadowOffsetWorld;

        float3 shadowUVWRaw = (shadowSamplePos - gFluidSettings.gridMin) / fluidSize;
        float3 shadowUVW = shadowUVWRaw - floor(shadowUVWRaw);

        float3 sDistToMin = shadowSamplePos - gFluidSettings.gridMin;
        float3 sDistToMax = gFluidSettings.gridMax - shadowSamplePos;
        float3 sMinDist = min(sDistToMin, sDistToMax);
        float shadowEdgeFade = smoothstep(0.0f, fluidSize.x * 0.1f, min(min(sMinDist.x, sMinDist.y), sMinDist.z));

        fluidShadowMass = max(gFluidDensity.SampleLevel(gSampler, shadowUVW, 0).r, 0.0f) * shadowEdgeFade;
    }

    // ---------------------------------------------------------
    // ボリューメトリックノイズの合成
    // ---------------------------------------------------------
    float3 timeOffset = normalize(gFogSettings.windDirection + kMinSafeDistance) * (gFrameData.gTime * gFogSettings.windSpeed);
    float3 noiseSamplePos = lerp(currentPos, advectedFluidPos, edgeFade);

    float3 warpUVW = noiseSamplePos * (gFogSettings.noiseScale * 0.43f) + timeOffset * 0.35f;
    float3 distortion = float3(
        gNoiseVolume.SampleLevel(gSampler, warpUVW, 0).r,
        gNoiseVolume.SampleLevel(gSampler, warpUVW + kNoiseOffsetA, 0).r,
        gNoiseVolume.SampleLevel(gSampler, warpUVW + kNoiseOffsetB, 0).r
    );

    float3 distortedPos = noiseSamplePos + (distortion * 2.0f - 1.0f) * gFogSettings.noiseDistortion;

    // Perlinによる基本骨格 (低・中周波) の合成
    float3 uvwA = distortedPos * gFogSettings.noiseScale + timeOffset;
    float4 noiseLayer1 = gNoiseVolume.SampleLevel(gSampler, uvwA, 0);
    
    float3 uvwB = distortedPos * (gFogSettings.noiseScale * 0.37f) + (timeOffset * 1.41f) + kNoiseOffsetC;
    float4 noiseLayer2 = gNoiseVolume.SampleLevel(gSampler, uvwB, 0);

    float combinedPerlin = noiseLayer1.r * noiseLayer2.r * 1.5f;

    // 遠景でのサンプリングエイリアス（チラつき）を防ぐため、距離に応じて高周波ノイズを減衰
    float linearDistanceRatio = saturate(sampleViewZ / farZ);
    float detailFade = smoothstep(0.1f, 0.6f, linearDistanceRatio);

    float activeWorleyWeight = lerp(gFogSettings.worleyWeight, 0.0f, detailFade);
    float activeErosion = lerp(gFogSettings.erosion, 0.0f, detailFade);
    float activeNoiseIntensity = lerp(gFogSettings.noiseIntensity, gFogSettings.noiseIntensity * 0.2f, detailFade);

    // Worleyノイズを用いてカリフラワー状のディテールを削り出す
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

    float finalGlobalDensity = globalBaseDensity * finalNoiseModifier;
    float particleDensity = max(finalGlobalDensity, fluidMass);

    // ---------------------------------------------------------
    // ライティングと光の減衰 (位相関数を含む)
    // ---------------------------------------------------------
    float dynamicShadowFog = max(globalBaseDensity * combinedNoiseEffect, fluidShadowMass);
    float fluidSelfShadow = exp(-dynamicShadowFog * 4.0f);
    float finalShadowVisibility = shadowVisibility * fluidSelfShadow;

    // Dual-Lobe Henyey-Greenstein を用いてMie散乱（前方への強い散乱）をシミュレート
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

        // ボクセルの粗さに起因するブロックノイズを防ぐため、厚みベースで距離を補正
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
        
        // 光源中心における特異点（極端な白飛び）を回避するためのブレンド処理
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

    float global_sigma_e = max(particleDensity * gFogSettings.extinctionScale, 0.00001f);
    float3 global_sigma_s = global_sigma_e * gFogSettings.albedo;

    float3 volumeScattering = 0;
    float volumeExtinction = 0;

    for (uint v = 0; v < gFogVolumeBuffer.volumeCount; ++v)
    {
        FogVolume vol = gFogVolumeBuffer.volumes[v];
        float3 distortedWorldPos = lerp(currentPos, advectedFluidPos, vol.distortionAmount);
        
        // 逆行列を用いてボリュームのローカル空間に変換し、判定を簡略化
        float3 localPos = mul(float4(distortedWorldPos, 1.0f), vol.worldToLocal).xyz;

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
            float3 volTimeOffset = normalize(vol.windDirection + kMinSafeDistance) * (gFrameData.gTime * vol.windSpeed);
            float3 baseVolPos = lerp(currentPos, noiseSamplePos, vol.distortionAmount);

            float3 volWarpUVW = (baseVolPos * vol.noiseScale * 0.4f) + volTimeOffset * 0.5f;
            float3 volWarp = gNoiseVolume.SampleLevel(gSampler, volWarpUVW, 0.0f).rgb * 2.0f - 1.0f;
            float3 volNoisePos = (baseVolPos * vol.noiseScale) + volTimeOffset + (volWarp * vol.distortionAmount);

            float4 volNoiseSample = gNoiseVolume.SampleLevel(gSampler, volNoisePos, 0.0f);

            float vNoise = lerp(volNoiseSample.r, 1.0f - volNoiseSample.g, vol.worleyWeight);
            vNoise = saturate(vNoise - (1.0f - vNoise) * volNoiseSample.b * vol.erosion);

            float volCutoff = 1.0f - (vol.coverage * volumeMask);
            float shiftedNoise = vNoise + vol.densityOffset;
            float volCoverage = smoothstep(volCutoff, volCutoff + max(vol.noiseFeather, kMinSafeDistance), shiftedNoise);

            float erosionFactor = saturate(1.0f - (1.0f - shiftedNoise) * max(vol.noiseContrast, 1.0f));
            float noiseModifier = lerp(1.0f, erosionFactor * volCoverage, vol.noiseIntensity);

            float localUVW_Y = localPos.y * 0.5f + 0.5f;
            float finalVolDensity = vol.density * exp(-localUVW_Y * max(vol.heightFalloff, 0.0f)) * noiseModifier * volumeMask;

            float finalVolShadowVis = shadowVisibility * exp(-finalVolDensity * 4.0f);
            float phaseVol = DualPhaseHG(cosTheta, vol.anisotropy);
            
            float3 volLight = finalVolShadowVis * phaseVol * gFrameData.mainLightColor.rgb;
            volLight += gFogSettings.ambientLight * lerp(0.3f, 1.0f, finalVolShadowVis);
            volLight += stepLocal;

            volumeExtinction += finalVolDensity * gFogSettings.extinctionScale;
            volumeScattering += vol.color * finalVolDensity * gFogSettings.scatteringIntensity * volLight;
        }
    }

    // ---------------------------------------------------------
    // ボクセルへの書き込み
    // ---------------------------------------------------------
    // 次段のレイマーチング用に、散乱光と消散係数をパック
    float3 scattering = ((totalLight * global_sigma_s) + volumeScattering) * depthWeight;
    float extinction = (global_sigma_e + volumeExtinction) * depthWeight;

    gVoxelInject[DTid.xyz] = float4(scattering, extinction);
}