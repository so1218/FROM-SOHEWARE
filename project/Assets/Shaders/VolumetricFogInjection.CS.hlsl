#include "ShaderConstants.hlsli"
#include "Object3D.hlsli"

Texture2D<float> gDepthTexture : register(t0);
Texture2D<float> gShadowMap : register(t1);
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

// 位相関数
// 光が霧の粒子にぶつかった時に、どの方向にどのくらい散乱するか
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

// レイマーチングのアーティファクトを消すためのノイズ関数
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
        
        // 流体データの取得
        float3 fluidSize = gFluidSettings.gridMax - gFluidSettings.gridMin;

        // スナップされたシミュレーション境界で判定
        bool isInsideFluidGrid = all(currentPos >= gFluidSettings.gridMin) && all(currentPos <= gFluidSettings.gridMax);

        float3 realFluidVelocity = 0.0f;
        float fluidMass = 0.0f;
        float3 advectedFluidPos = currentPos;
        float fluidShadowMass = 0.0f;
        float edgeFade = 0.0f;

        if (isInsideFluidGrid)
        {
            // 境界付近の3軸フェード
            float3 distToMin = currentPos - gFluidSettings.gridMin;
            float3 distToMax = gFluidSettings.gridMax - currentPos;
            float3 minDist = min(distToMin, distToMax);
            edgeFade = smoothstep(0.0f, fluidSize.x * 0.1f, min(min(minDist.x, minDist.y), minDist.z));

            // 負の座標に対応したトーラスマッピング (0.0～1.0へのWrap)
            float3 uvwRaw = currentPos / fluidSize;
            float3 fluidUVW = uvwRaw - floor(uvwRaw);

            // Wrapサンプラーでサンプリング
            realFluidVelocity = gFluidVelocity.SampleLevel(gSampler, fluidUVW, 0).xyz * edgeFade;
            fluidMass = max(gFluidDensity.SampleLevel(gSampler, fluidUVW, 0).r, 0.0f) * edgeFade;

            // 移流（Advection）されたUVWを取得
            float3 advectedUVW = gFluidUVW.SampleLevel(gSampler, fluidUVW, 0).xyz;
            float3 uvwOffset = advectedUVW - fluidUVW;

            // トーラスラップ境界のオフセットを最短経路補正
            uvwOffset = uvwOffset - floor(uvwOffset + 0.5f);

            // オフセットから歪んだワールド座標を算出
            advectedFluidPos = currentPos + (uvwOffset * fluidSize);

            // セルフシャドウ用サンプリング
            float3 shadowOffsetWorld = normalize(-gFrameData.mainLightDirection) * (2.0f * gFluidSettings.gridScale);
            float3 shadowSamplePos = currentPos + shadowOffsetWorld;

            // floorによるToroidal Wrapでループサンプリング
            float3 shadowUVWRaw = (shadowSamplePos - gFluidSettings.gridMin) / fluidSize;
            float3 shadowUVW = shadowUVWRaw - floor(shadowUVWRaw);
            
            // シャドウ用フェードの計算
            float3 sDistToMin = shadowSamplePos - gFluidSettings.gridMin;
            float3 sDistToMax = gFluidSettings.gridMax - shadowSamplePos;
            float3 sMinDist = min(sDistToMin, sDistToMax);
            float shadowEdgeFade = smoothstep(0.0f, fluidSize.x * 0.1f, min(min(sMinDist.x, sMinDist.y), sMinDist.z));

            fluidShadowMass = max(gFluidDensity.SampleLevel(gSampler, shadowUVW, 0).r, 0.0f) * shadowEdgeFade;
        }

        // 風による時間のオフセット
        float3 timeOffset = normalize(gFogSettings.windDirection + 0.001f) * (gFrameData.gTime * gFogSettings.windSpeed);

        // ノイズサンプリング座標
        float3 noiseSamplePos = lerp(currentPos, advectedFluidPos, edgeFade);
        
        // 3Dノイズによる微細な歪み
        float3 warpUVW = noiseSamplePos * (gFogSettings.noiseScale * 0.43f) + timeOffset * 0.35f;
        float3 distortion = float3(
            gNoiseVolume.SampleLevel(gSampler, warpUVW, 0).r,
            gNoiseVolume.SampleLevel(gSampler, warpUVW + float3(131.0f, 73.0f, 191.0f), 0).r,
            gNoiseVolume.SampleLevel(gSampler, warpUVW + float3(43.0f, 269.0f, 113.0f), 0).r
        );

        // 歪みを与えた最終的なワールド座標
        float3 distortedPos = noiseSamplePos + (distortion * 2.0f - 1.0f) * gFogSettings.noiseDistortion;
        
        // 多重ノイズの合成
        // ベースとなる大きな雲（低周波）
        float3 uvwA = distortedPos * gFogSettings.noiseScale + timeOffset;
        float4 noiseLayer1 = gNoiseVolume.SampleLevel(gSampler, uvwA, 0);
        float basePerlin1 = noiseLayer1.r; // Perlin

        // 別のスケールと別の速度で動く大きな雲（中周波）
        float3 uvwB = distortedPos * (gFogSettings.noiseScale * 0.37f) + (timeOffset * 1.41f) + float3(411.0f, 823.0f, 157.0f);
        float4 noiseLayer2 = gNoiseVolume.SampleLevel(gSampler, uvwB, 0);
        float basePerlin2 = noiseLayer2.r; // 別周期のPerlin

        // 2つの異なる周期のPerlinを掛け合わせる
        float combinedPerlin = basePerlin1 * basePerlin2 * 1.5f;

        // ディテールとエロージョン
        float gWorley = noiseLayer1.g; // 粗いディテール
        float bWorley = noiseLayer1.b; // 細かいディテール

        // Perlinのブレンド結果に対してWorleyによるモコモコ感と削りを適用
        float combinedNoise = lerp(combinedPerlin, 1.0f - gWorley, gFogSettings.worleyWeight);
        combinedNoise = saturate(combinedNoise - (bWorley * gFogSettings.erosion));

        // 流体密度とノイズの融合
        float cutoff = 1.0f - gFogSettings.coverage;
        float feather = max(gFogSettings.noiseFeather, 0.001f);
        
        // 高さ減衰を計算
        float heightFactor = 1.0f;
        if (gFogSettings.heightFalloff > 0.0f)
        {
            heightFactor = exp(-max(currentPos.y - gFogSettings.baseHeight, 0.0f) * gFogSettings.heightFalloff);
        }
        
        // 密度のブレンド
        float globalBaseDensity = gFogSettings.globalDensity + (gFogSettings.heightDensity * heightFactor);
        float fluidBaseDensity = fluidMass;

        float blendWeight = saturate(fluidMass * edgeFade);
        float macroDensity = lerp(globalBaseDensity, max(globalBaseDensity, fluidBaseDensity), blendWeight);

        // ミクロディテール(Noise/Erosion)の適用
        float noiseCoverage = smoothstep(cutoff, cutoff + feather, combinedNoise);
        
        // 乗算型のエロージョン計算
        float erosionFactor = saturate(1.0f - (1.0f - combinedNoise) * gFogSettings.erosionStrength);
        
        // ノイズ効果のまとめとブレンド
        float combinedNoiseEffect = erosionFactor * noiseCoverage;
        float finalNoiseModifier = lerp(1.0f, combinedNoiseEffect, gFogSettings.noiseIntensity);
        
        // 最終的なパーティクル密度の決定
        float particleDensity = macroDensity * finalNoiseModifier;

        // セルフシャドウ用密度の計算
        float combinedShadowBase = lerp(globalBaseDensity, max(globalBaseDensity, fluidShadowMass), saturate(fluidShadowMass * edgeFade));
        float dynamicShadowFog = combinedShadowBase * erosionFactor * noiseCoverage;

        // 光の減衰とライティング計算
        float fluidSelfShadow = exp(-dynamicShadowFog * 4.0f);

        // 受光量の計算（地形シャドウ × 流体セルフシャドウ を合成）
        float finalShadowVisibility = shadowVisibility * fluidSelfShadow;

        float cosTheta = dot(rayDir, normalize(-gFrameData.mainLightDirection));
        float phase = DualPhaseHG(cosTheta, gFogSettings.anisotropy);
        
        float3 totalLight = finalShadowVisibility * phase * gFrameData.mainLightColor.rgb;
        totalLight += gFogSettings.ambientLight * lerp(0.3f, 1.0f, finalShadowVisibility);

        // ローカルライトの累積用
        float3 stepLocal = 0;

        // ローカルライト用の煙による減衰
        float localFogAttenuation = exp(-particleDensity * 2.0f);

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

            // 物理ベースの距離減衰
            float distanceFalloff = 1.0f / (max(distSq, 0.01f) + 1.0f);
            float windowing = saturate(1.0f - pow(distSq / radiusSq, 2.0f));
            float attenuation = distanceFalloff * (windowing * windowing);

            // 異方性の適用
            float phaseLocal = DualPhaseHG(dot(rayDir, lightVec / dist), gFogSettings.anisotropy);

            // ライトの適用
            stepLocal += gPointLights[p].color.rgb * gPointLights[p].intensity * attenuation * phaseLocal * localFogAttenuation;
        }

        for (int s = 0; s < MAX_SPOT_LIGHTS; ++s)
        {
            if (gSpotLights[s].enable == 0)
                continue;
            
            float3 lightVec = gSpotLights[s].position - currentPos;
            
            // 距離計算
            float distance = length(lightVec);
            
            if (distance > gSpotLights[s].distance)
                continue;
            
            float3 lDir = lightVec / distance; // ボクセルから光源への方向
            float currentCos = dot(-lDir, normalize(gSpotLights[s].direction));
            
            if (currentCos < gSpotLights[s].cosAngle)
                continue;
            
            // 距離減衰
            float distanceAtt = gSpotLights[s].distance > 0.0001f
                ? pow(saturate(1.0f - distance / gSpotLights[s].distance), gSpotLights[s].decay)
                : 1.0f;
            
            // 角度減衰
            float angleAtt = pow(saturate((currentCos - gSpotLights[s].cosAngle) / (1.0f - gSpotLights[s].cosAngle)), 2.0f);
            
            float attenuation = distanceAtt * angleAtt;
            
            // フォグの位相関数
            float phaseLocal = DualPhaseHG(dot(rayDir, lDir), gFogSettings.anisotropy);
            
            // ボリューム用輝度ブースト
            float volumetricScatteringIntensity = 1.0f;
            
            // 煙による遮蔽
            float spotLocalFogAttenuation = exp(-particleDensity * 1.0f);

            // 最終合成
            stepLocal += gSpotLights[s].color.rgb * (gSpotLights[s].intensity * volumetricScatteringIntensity) * attenuation * phaseLocal * spotLocalFogAttenuation;
        }

        totalLight += stepLocal;
        
        // 距離フェードの適用
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
            float3 distortedWorldPos = lerp(currentPos, advectedFluidPos, vol.distortionAmount);
            float3 localPos = mul(float4(distortedWorldPos, 1.0f), vol.worldToLocal).xyz;
            
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
                
                // サンプリング座標の計算
                float3 baseVolPos = lerp(currentPos, noiseSamplePos, vol.distortionAmount);
                float3 volWarpUVW = (baseVolPos * vol.noiseScale * 0.4f) + volTimeOffset * 0.5f;
                float3 volWarp = gNoiseVolume.SampleLevel(gSampler, frac(volWarpUVW), 0.0f).rgb * 2.0f - 1.0f;
                float3 volNoisePos = (baseVolPos * vol.noiseScale) + volTimeOffset + (volWarp * vol.distortionAmount);

                float4 volNoiseSample = gNoiseVolume.SampleLevel(gSampler, frac(volNoisePos), 0.0f);
                float volBase = volNoiseSample.r;
                float volCoarseErode = volNoiseSample.g;
                float volFineErode = volNoiseSample.b;

                // マスク主導型エロージョン
                
                // ベースノイズ合成
                float vNoise = lerp(volBase, 1.0f - volCoarseErode, vol.worleyWeight);
                vNoise = saturate(vNoise - (1.0f - vNoise) * volFineErode * vol.erosion);

                // カバレッジ計算
                float volCutoff = 1.0f - (vol.coverage * volumeMask);
                float volFeather = max(vol.noiseFeather, 0.001f);
                
                // オフセットを足してから切り出す
                float shiftedNoise = vNoise + vol.densityOffset;
                float volCoverage = smoothstep(volCutoff, volCutoff + volFeather, shiftedNoise);

                // 乗算型エロージョン
                float erosionFactor = saturate(1.0f - (1.0f - shiftedNoise) * max(vol.noiseContrast, 1.0f));
                float combinedNoiseEffect = erosionFactor * volCoverage;
                
                // インテンシティによる最終変調
                float noiseModifier = lerp(1.0f, combinedNoiseEffect, vol.noiseIntensity);

                // 最終密度の決定
                float localUVW_Y = localPos.y * 0.5f + 0.5f;
                float volHeightFactor = exp(-localUVW_Y * max(vol.heightFalloff, 0.0f));
                
                float finalVolDensity = vol.density * volHeightFactor * noiseModifier * volumeMask;

                // ボリュームのライティング
                float volSelfShadow = exp(-finalVolDensity * 4.0f);
                float finalVolShadowVis = shadowVisibility * volSelfShadow;

                float phaseVol = DualPhaseHG(cosTheta, vol.anisotropy);
                float3 volLight = finalVolShadowVis * phaseVol * gFrameData.mainLightColor.rgb;
    
                volLight += gFogSettings.ambientLight * lerp(0.3f, 1.0f, finalVolShadowVis);
                volLight += stepLocal;

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