#include "ShaderConstants.hlsli"
#include "Object3D.hlsli"

Texture2D<float> gDepthTexture : register(t0);
Texture2D<float> gShadowMap : register(t1);
Texture3D<float4> gNoiseVolume : register(t2);
Texture3D<float> gFluidDensity : register(t3);
Texture3D<float4> gFluidVelocity : register(t4);
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
    
    float hwDepth = gDepthTexture.SampleLevel(gSampler, float2(screenU, screenV), 0).r;
    float4 sceneWorld = mul(float4(clipX, clipY, hwDepth, 1.0f), gFrameData.invViewProj);
    sceneWorld.xyz /= sceneWorld.w;
    float sceneDist = length(sceneWorld.xyz - gFrameData.cameraWorldPosition);

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
        
        // ===============================================================
        // ★修正：流体データを取得し、シームレスなフェードをかける
        // ===============================================================
        float3 fluidUVW = (currentPos - gFluidSettings.gridMin) / (gFluidSettings.gridMax - gFluidSettings.gridMin);
        float3 realFluidVelocity = 0.0f;
        float realFluidDensity = 0.0f;
        
        if (all(fluidUVW >= 0.0f) && all(fluidUVW <= 1.0f))
        {
            float3 edgeDist = min(fluidUVW, 1.0f - fluidUVW);
            float edgeFade = smoothstep(0.0f, 0.1f, min(min(edgeDist.x, edgeDist.y), edgeDist.z));

            realFluidVelocity = gFluidVelocity.SampleLevel(gSampler, fluidUVW, 0).xyz * edgeFade;
            realFluidDensity = gFluidDensity.SampleLevel(gSampler, fluidUVW, 0).r * edgeFade; // 穴があるほどマイナス値
        }

        // 風による時間のオフセット
        float3 timeOffset = normalize(gFogSettings.windDirection + 0.001f) * (gFrameData.gTime * gFogSettings.windSpeed);

       // ===============================================================
        // ★大改造1: 流速だけでなく「穴の深さ」もうねりに変えて、座標を立体的にねじ曲げる
        // ===============================================================
        float3 fluidDistortion = realFluidVelocity * 0.7f + float3(realFluidDensity, -realFluidDensity, length(realFluidVelocity)) * 0.3f;
        float3 noiseSamplePos = currentPos - fluidDistortion;

        // グローバルフォグ用の微細な歪み（既存の3Dノイズボリュームによる揺らぎ）
        float3 warpUVW = noiseSamplePos * (gFogSettings.noiseScale * 0.5f) + timeOffset * 0.5f;
        float3 distortion = float3(
            gNoiseVolume.SampleLevel(gSampler, frac(warpUVW), 0).r,
            gNoiseVolume.SampleLevel(gSampler, frac(warpUVW + 0.33f), 0).r,
            gNoiseVolume.SampleLevel(gSampler, frac(warpUVW + 0.67f), 0).r
        );

        float3 distortedPos = noiseSamplePos + (distortion * 2.0f - 1.0f) * gFogSettings.noiseDistortion;
        float coreMask = 1.0f;
        
// 多重ノイズの合成 (basePerlin, detailNoise から combinedNoise を作るところは共通)
        float3 uvwA = distortedPos * gFogSettings.noiseScale + timeOffset;
        float basePerlin = gNoiseVolume.SampleLevel(gSampler, frac(uvwA), 1.0f).r;
        float3 uvwB = distortedPos.zxy * (gFogSettings.noiseScale * 2.0f) - (timeOffset * 0.7f) + float3(0.31f, 0.74f, 0.12f);
        float4 detailNoise = gNoiseVolume.SampleLevel(gSampler, frac(uvwB), 0);
        
        float combinedNoise = lerp(basePerlin, 1.0f - detailNoise.g, gFogSettings.worleyWeight);
        combinedNoise = saturate(combinedNoise - (detailNoise.b * gFogSettings.erosion));

        // ===============================================================
        // ★大改造2: 流体密度（負の値）を単に足すのではなく、ノイズの「Coverage」を侵食する！
        // ===============================================================
        // realFluidDensityはマイナス値なので、正の数に反転して侵食強度(Erosion)にする
        float fluidErosion = saturate(-realFluidDensity * 1.5f);

        // くり抜き閾値(cutoff)に流体の侵食度を足すことで、ノイズの「濃い芯の部分」だけを残してディテールに沿って削る
        float cutoff = (1.0f - gFogSettings.coverage) + fluidErosion;
        float feather = max(gFogSettings.noiseFeather, 0.001f);
        float noiseVal = smoothstep(cutoff, cutoff + feather, combinedNoise);
        
        // 受光量の計算
        float cosTheta = dot(rayDir, normalize(-gFrameData.mainLightDirection));
        float phase = DualPhaseHG(cosTheta, gFogSettings.anisotropy);
        
        float3 totalLight = shadowVisibility * phase * gFrameData.mainLightColor.rgb;
        totalLight += gFogSettings.ambientLight * lerp(0.3f, 1.0f, shadowVisibility);

        // [ローカルライト（ポイント/スポット）の計算は変更がないため中身を維持]
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
        totalLight += stepLocal;
        
// ===============================================================
        // 1. グローバルフォグの基本密度を計算
        // ===============================================================
      // 最終的なグローバルフォグの密度計算
        float heightFactor = exp(-max(currentPos.y - gFogSettings.baseHeight, 0.0f) * gFogSettings.heightFalloff);
        float particleDensity = gFogSettings.globalDensity + (gFogSettings.heightDensity * heightFactor * noiseVal * coreMask);

        // オブジェクトの完全な中心部が100%スカスカになるように念のための補助補正
        particleDensity = max(particleDensity + min(realFluidDensity, 0.0f) * 0.1f, 0.0f);

        // ===============================================================
        // 3. 最後に距離フェードなどをかける
        // ===============================================================
        float fadeStart = farZ * 0.8f;
        float distanceFade = saturate((farZ - sampleViewZ) / max(farZ - fadeStart, 0.001f));
        particleDensity *= distanceFade;

        float3 global_sigma_s = gFogSettings.scatteringColor * particleDensity * gFogSettings.scatteringIntensity;
        float global_sigma_e = max(particleDensity * gFogSettings.extinctionScale, 0.00001f);

        // ---------------------------------------------------------------
        // 2. 配置式フォグボリュームの計算（★劇的アップグレード部分）
        // ---------------------------------------------------------------
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
                
                // ★修正：配置式フォグにも流体の歪み（noiseSamplePos）を適用する！
                // currentPos ではなく noiseSamplePos をベースにすることで、配置した霧も一緒に渦を巻きます
                float3 baseVolPos = lerp(currentPos, noiseSamplePos, vol.distortionAmount);

                float3 volWarpUVW = (baseVolPos * vol.noiseScale * 0.4f) + volTimeOffset * 0.5f;
                float3 volWarp = gNoiseVolume.SampleLevel(gSampler, volWarpUVW, 0.0f).rgb * 2.0f - 1.0f;
    
    // ワールド座標ベースで座標を確定
                float3 volNoisePos = (baseVolPos * vol.noiseScale) + volTimeOffset + (volWarp * vol.distortionAmount);

    // ★修正2: MipLevelを1.0から0.0（最高解像度）へ変更！Worleyのエッジを完全に活かす。frac()を削除
                float4 volNoiseSample = gNoiseVolume.SampleLevel(gSampler, volNoisePos, 0.0f);
    
                float volBase = volNoiseSample.r; // R: 大きなうねり
                float volCoarseErode = volNoiseSample.g; // G: 中Worley
                float volFineErode = volNoiseSample.b; // B: 小Worley
                float volMicroDetail = volNoiseSample.a; // A: 極小Worley

    // ノイズの合成
                float volCombinedNoise = lerp(volBase, 1.0f - volCoarseErode, vol.worleyWeight);
                volCombinedNoise = saturate(volCombinedNoise - (volFineErode * vol.erosion));

    // コントラストとオフセットの適用
                volCombinedNoise = saturate((volCombinedNoise + vol.densityOffset) * vol.noiseContrast);

    // ===============================================================
            // ★大改造3: 配置式フォグの形状切り出し(Coverage)も同様に流体侵食をかける
            // ===============================================================
                float volFluidErosion = saturate(-realFluidDensity * vol.distortionAmount * 1.5f);
                float volCutoff = (1.0f - vol.coverage) + volFluidErosion;
                float volFeather = max(vol.noiseFeather, 0.001f);
                float volNoiseVal = smoothstep(volCutoff, volCutoff + volFeather, volCombinedNoise);

                float noiseModifier = lerp(1.0f, volNoiseVal, vol.noiseIntensity);
                float finalVolDensity = vol.density * noiseModifier * volumeMask;

    // ローカル高さ減衰
                float localUVW_Y = localPos.y * 0.5f + 0.5f;
                float volHeightFactor = exp(-localUVW_Y * max(vol.heightFalloff, 0.0f));
                finalVolDensity *= volHeightFactor;

    // ★修正4: セルフシャドウ（吸光）の強化
    // 密度に応じて光を遮ることで、雲の「影の付いた下腹部」のような重厚感が出ます。
                float selfShadow = exp(-finalVolDensity * 4.0f); // 遮蔽強度を4.0に強化

    // ライティングの計算
                float phaseVol = DualPhaseHG(cosTheta, vol.anisotropy);
                float3 volLight = shadowVisibility * phaseVol * gFrameData.mainLightColor.rgb * selfShadow;
    
                volLight += gFogSettings.ambientLight * lerp(0.1f, 1.0f, shadowVisibility * selfShadow);
                volLight += stepLocal * selfShadow;

                volumeExtinction += finalVolDensity * gFogSettings.extinctionScale;
                volumeScattering += vol.color * finalVolDensity * gFogSettings.scatteringIntensity * volLight;
            }
        }

        // グローバルとボリュームの合算・累積
        float3 sampleScattering = (totalLight * global_sigma_s) + volumeScattering;
        float sampleExtinction = global_sigma_e + volumeExtinction;

        float fadeRange = max(voxelThickness * 1.0f, 0.1f);
        float depthWeight = saturate((sceneDist - sampleViewZ) / fadeRange);

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