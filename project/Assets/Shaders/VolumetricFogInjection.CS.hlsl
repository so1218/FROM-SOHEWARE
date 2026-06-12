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
        // ★流体データの取得
        // ===============================================================
        float3 fluidSize = gFluidSettings.gridMax - gFluidSettings.gridMin;
        float3 fluidUVW = (currentPos - gFluidSettings.gridMin) / fluidSize;
        float3 realFluidVelocity = 0.0f;
        float fluidMass = 0.0f;
        float3 advectedFluidUVW = fluidUVW; // デフォルトは歪みなしのUVW
        
        float fluidShadowMass = 0.0f;
        float edgeFade = 0.0f; // ★ループ外でも使えるようにここで宣言

        if (all(fluidUVW >= 0.0f) && all(fluidUVW <= 1.0f))
        {
            float3 edgeDist = min(fluidUVW, 1.0f - fluidUVW);
            edgeFade = smoothstep(0.0f, 0.1f, min(min(edgeDist.x, edgeDist.y), edgeDist.z));

            realFluidVelocity = gFluidVelocity.SampleLevel(gSampler, fluidUVW, 0).xyz * edgeFade;
            fluidMass = max(gFluidDensity.SampleLevel(gSampler, fluidUVW, 0).r, 0.0f) * edgeFade;

            // ★修正：C++側で毎フレームシミュレーションした「移流蓄積されたUVW座標」をサンプリング
            advectedFluidUVW = gFluidUVW.SampleLevel(gSampler, fluidUVW, 0).xyz;

            // ===============================================================
            // ★大改造4: 疑似ボリューメトリック・セルフシャドウ（直方体バグ修正版）
            // ===============================================================
            float3 shadowOffsetWorld = normalize(-gFrameData.mainLightDirection) * (2.0f * gFluidSettings.gridScale);
            float3 shadowUVW = fluidUVW + (shadowOffsetWorld / fluidSize);
            
            if (all(shadowUVW >= 0.0f) && all(shadowUVW <= 1.0f))
            {
                fluidShadowMass = max(gFluidDensity.SampleLevel(gSampler, shadowUVW, 0).r, 0.0f) * edgeFade;
            }
        }

        // 風による時間のオフセット
        float3 timeOffset = normalize(gFogSettings.windDirection + 0.001f) * (gFrameData.gTime * gFogSettings.windSpeed);

      // ===============================================================
        // ★大改造1: 流速(Velocity)ではなく、蓄積されたUVWによる「本物の流体移流」
        // ===============================================================
        float3 fluidDistortedPos = gFluidSettings.gridMin + advectedFluidUVW * fluidSize;
        
        // 【★修正1】不連続バグの解消
        // 密度(fluidMass)ベースの3項演算子をやめ、グリッドの境界フェード(edgeFade)で滑らかに座標をブレンドします。
        // これにより、グリッドの内外でノイズのサンプリング位置がジャンプする現象が完全に消え、滑らかに繋がります。
        // また、煙の密度が0になっても「空間の歪み（過去の渦）」がグリッド内に正しく残り続けるようになります。
        float3 noiseSamplePos = lerp(currentPos, fluidDistortedPos, edgeFade);

        // 微細な歪み（既存の3Dノイズボリュームによる揺らぎ）
        float3 warpUVW = noiseSamplePos * (gFogSettings.noiseScale * 0.5f) + timeOffset * 0.5f;
        float3 distortion = float3(
            gNoiseVolume.SampleLevel(gSampler, frac(warpUVW), 0).r,
            gNoiseVolume.SampleLevel(gSampler, frac(warpUVW + 0.33f), 0).r,
            gNoiseVolume.SampleLevel(gSampler, frac(warpUVW + 0.67f), 0).r
        );

        float3 distortedPos = noiseSamplePos + (distortion * 2.0f - 1.0f) * gFogSettings.noiseDistortion;
        
        // 多重ノイズの合成
        float3 uvwA = distortedPos * gFogSettings.noiseScale + timeOffset;
        float basePerlin = gNoiseVolume.SampleLevel(gSampler, frac(uvwA), 1.0f).r;
        float3 uvwB = distortedPos.zxy * (gFogSettings.noiseScale * 2.0f) - (timeOffset * 0.7f) + float3(0.31f, 0.74f, 0.12f);
        float4 detailNoise = gNoiseVolume.SampleLevel(gSampler, frac(uvwB), 0);
        
        float combinedNoise = lerp(basePerlin, 1.0f - detailNoise.g, gFogSettings.worleyWeight);
        combinedNoise = saturate(combinedNoise - (detailNoise.b * gFogSettings.erosion));

       // ===============================================================
        // ★大改造2: 流体密度(マクロ) × ノイズ(ミクロ) の完全融合
        // ===============================================================
        float cutoff = 1.0f - gFogSettings.coverage;
        float feather = max(gFogSettings.noiseFeather, 0.001f);
        
        // 1. 先に高さ減衰（Height Factor）を計算
        float heightFactor = 1.0f;
        if (gFogSettings.heightFalloff > 0.0f)
        {
            heightFactor = exp(-max(currentPos.y - gFogSettings.baseHeight, 0.0f) * gFogSettings.heightFalloff);
        }
        
        // ===============================================================
        // ★大改造: Unreal / Frostbite基準の「密度合成（Density Compositing）」
        // ===============================================================

        // 1. マクロな形状（Base Density）を別々に計算する
        float globalBaseDensity = gFogSettings.globalDensity + (gFogSettings.heightDensity * heightFactor);
        float fluidBaseDensity = fluidMass;

        // 2. 密度の合成（Compositing）
        float blendWeight = saturate(fluidMass * edgeFade);
        float macroDensity = lerp(globalBaseDensity, max(globalBaseDensity, fluidBaseDensity), blendWeight);

        // 3. ミクロなディテール（Noise / Erosion）の適用
        float noiseCoverage = smoothstep(cutoff, cutoff + feather, combinedNoise);
        
        // 【★Unreal Engine基準の修正】：減算型から「乗算（比率）型」のエロージョンに変更
        // combinedNoise(0～1) に応じて、ベース密度をどれだけの割合削るかを計算します。
        // これにより、密度の高い流体内部も、密度の低い外側も、同じコントラストで均一に削り出されます。
        float erosionFactor = saturate(1.0f - (1.0f - combinedNoise) * gFogSettings.erosionStrength);
        
   // ★【修正】ノイズの効果を1つにまとめ、Intensityで「1.0（無地）」とブレンドする
        float combinedNoiseEffect = erosionFactor * noiseCoverage;
        float finalNoiseModifier = lerp(1.0f, combinedNoiseEffect, gFogSettings.noiseIntensity);
        
        // 4. 最終的なパーティクル密度
        // finalNoiseModifierが1.0なら、macroDensityがそのまま出力され完全なのっぺりフォグになる
        float particleDensity = macroDensity * finalNoiseModifier;

        // 【シャドウの計算連動】セルフシャドウ用の密度も同様に比率で計算
        float combinedShadowBase = lerp(globalBaseDensity, max(globalBaseDensity, fluidShadowMass), saturate(fluidShadowMass * edgeFade));
        float dynamicShadowFog = combinedShadowBase * erosionFactor * noiseCoverage;


        // ===============================================================
        // ★大改造5: 光の減衰（Beer-Lambert則）の適用 と ライティング
        // 【修正】密度が確定した後に、受光量を計算する！
        // ===============================================================
        
        // 直上で合成・計算した dynamicShadowFog を使って遮蔽率を出す
        float fluidSelfShadow = exp(-dynamicShadowFog * 4.0f);

        // 受光量の計算（地形シャドウ × 流体セルフシャドウ を合成）
        float finalShadowVisibility = shadowVisibility * fluidSelfShadow;

        float cosTheta = dot(rayDir, normalize(-gFrameData.mainLightDirection));
        float phase = DualPhaseHG(cosTheta, gFogSettings.anisotropy);
        
        float3 totalLight = finalShadowVisibility * phase * gFrameData.mainLightColor.rgb;
        totalLight += gFogSettings.ambientLight * lerp(0.3f, 1.0f, finalShadowVisibility);

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
        // 3. 最後に距離フェードなどをかける
        // ===============================================================
        float fadeStart = farZ * 0.8f;
        float distanceFade = saturate((farZ - sampleViewZ) / max(farZ - fadeStart, 0.001f));
        particleDensity *= distanceFade;

        float3 global_sigma_s = gFogSettings.scatteringColor * particleDensity * gFogSettings.scatteringIntensity;
        float global_sigma_e = max(particleDensity * gFogSettings.extinctionScale, 0.00001f);

        // ---------------------------------------------------------------
        // 2. 配置式フォグボリュームの計算
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
                
                // ★修正：配置式フォグにも流体の本物の歪み（noiseSamplePos）を適用
                float3 baseVolPos = lerp(currentPos, noiseSamplePos, vol.distortionAmount);

                float3 volWarpUVW = (baseVolPos * vol.noiseScale * 0.4f) + volTimeOffset * 0.5f;
                float3 volWarp = gNoiseVolume.SampleLevel(gSampler, volWarpUVW, 0.0f).rgb * 2.0f - 1.0f;
    
                float3 volNoisePos = (baseVolPos * vol.noiseScale) + volTimeOffset + (volWarp * vol.distortionAmount);

                float4 volNoiseSample = gNoiseVolume.SampleLevel(gSampler, volNoisePos, 0.0f);
    
                float volBase = volNoiseSample.r;
                float volCoarseErode = volNoiseSample.g;
                float volFineErode = volNoiseSample.b;

                float volCombinedNoise = lerp(volBase, 1.0f - volCoarseErode, vol.worleyWeight);
                volCombinedNoise = saturate(volCombinedNoise - (volFineErode * vol.erosion));
                volCombinedNoise = saturate((volCombinedNoise + vol.densityOffset) * vol.noiseContrast);

                float volCutoff = 1.0f - vol.coverage;
                float volFeather = max(vol.noiseFeather, 0.001f);
                float volNoiseVal = smoothstep(volCutoff, volCutoff + volFeather, volCombinedNoise);

                float noiseModifier = lerp(1.0f, volNoiseVal, vol.noiseIntensity);
                float finalVolDensity = vol.density * noiseModifier * volumeMask;

                float localUVW_Y = localPos.y * 0.5f + 0.5f;
                float volHeightFactor = exp(-localUVW_Y * max(vol.heightFalloff, 0.0f));
                finalVolDensity *= volHeightFactor;

                float selfShadow = exp(-finalVolDensity * 4.0f);

                float phaseVol = DualPhaseHG(cosTheta, vol.anisotropy);
                float3 volLight = shadowVisibility * phaseVol * gFrameData.mainLightColor.rgb * selfShadow;
    
                // ★修正：環境光・ローカルライトの遮蔽計算にバグが出ないようセルフシャドウを正しく乗算
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