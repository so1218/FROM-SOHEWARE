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

    // Dithered Lookups: 毎フレーム、インターリーブグラジエントノイズでサンプリング位置をずらす
    float noiseJitter = InterleavedGradientNoise(DTid.xy, gFrameData.frameIndex);
    
    // ループを完全に撤廃し、この1点のみを評価
    float t = noiseJitter;
    float sampleViewZ = viewZ0 + voxelThickness * t;

    float3 currentPos = gFrameData.cameraWorldPosition + (rayDir * sampleViewZ);

    // シャドウの計算
    // カスケードの判定（sampleViewZ は既に計算済みのカメラからのビュー深度）
    uint cascadeIndex = 0;
    if (sampleViewZ > gShadowData.cascadeSplits[1])
        cascadeIndex = 2;
    else if (sampleViewZ > gShadowData.cascadeSplits[0])
        cascadeIndex = 1;

    // 判定したカスケードの行列を使ってシャドウ座標を計算
    float4 shadowCoord = mul(float4(currentPos, 1.0f), gShadowData.cascadeLightViewProj[cascadeIndex]);
    shadowCoord.xyz /= shadowCoord.w;
    float2 shadowUV = shadowCoord.xy * float2(0.5f, -0.5f) + 0.5f;
    
    float shadowVisibility = 1.0f;
    if (all(shadowUV >= 0.0f) && all(shadowUV <= 1.0f) && shadowCoord.z >= 0.0f && shadowCoord.z <= 1.0f)
    {
    // float3(shadowUV.x, shadowUV.y, cascadeIndex) としてサンプリング
        shadowVisibility = gShadowMap.SampleCmpLevelZero(
        gShadowSampler,
        float3(shadowUV, cascadeIndex),
        shadowCoord.z - 0.0001f
    );
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
        float3 fluidUVW = frac(uvwRaw);

        // Wrapサンプラーでサンプリング
        realFluidVelocity = gFluidVelocity.SampleLevel(gSampler, fluidUVW, 0).xyz * edgeFade;
        fluidMass = max(gFluidDensity.SampleLevel(gSampler, fluidUVW, 0).r, 0.0f) * edgeFade;

        // 移流されたUVWを取得
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
    
    // 遠方のエイリアシング（チラつき）を防ぐためのディテールフェード
    float linearDistanceRatio = saturate(sampleViewZ / farZ);
    float detailFade = smoothstep(0.1f, 0.6f, linearDistanceRatio); // 実際の距離の10%〜60%でフェード

    // 遠方ほどWorley（モコモコ）とErosion（削り）のウェイトをゼロに落とす
    float activeWorleyWeight = lerp(gFogSettings.worleyWeight, 0.0f, detailFade);
    float activeErosion = lerp(gFogSettings.erosion, 0.0f, detailFade);
    float activeNoiseIntensity = lerp(gFogSettings.noiseIntensity, gFogSettings.noiseIntensity * 0.2f, detailFade);

    
    // Perlinのブレンド結果に対してWorleyによるモコモコ感と削りを適用
    float combinedNoise = lerp(combinedPerlin, 1.0f - gWorley, activeWorleyWeight);
    combinedNoise = saturate(combinedNoise - (bWorley * activeErosion));

    // ミクロディテール(Noise/Erosion)の適用
    float cutoff = 1.0f - gFogSettings.coverage;
    float feather = max(gFogSettings.noiseFeather, 0.001f);
    
    // 高さ減衰を計算
    float heightFactor = 1.0f;
    if (gFogSettings.heightFalloff > 0.0f)
    {
        // baseHeightより下（進入時）は 0 になり、exp(0)=1.0。上に行くほど薄くなる。
        float heightDiff = max(currentPos.y - gFogSettings.baseHeight, 0.0f);
        heightFactor = exp(-heightDiff * gFogSettings.heightFalloff);
    }

    // 密度の決定
    float globalBaseDensity = gFogSettings.extinction + (gFogSettings.heightDensity * heightFactor);

    // ミクロディテール(Noise/Erosion)の適用（グローバルフォグ専用）
    float noiseCoverage = smoothstep(cutoff, cutoff + feather, combinedNoise);
    float erosionFactor = saturate(1.0f - (1.0f - combinedNoise) * gFogSettings.erosionStrength);
    
    float combinedNoiseEffect = erosionFactor * noiseCoverage;
    float finalNoiseModifier = lerp(1.0f, combinedNoiseEffect, activeNoiseIntensity);
    
    // グローバルフォグの最終密度
    float finalGlobalDensity = globalBaseDensity * finalNoiseModifier;

    // 流体の密度
    float finalFluidDensity = fluidMass;

    // 最終的なパーティクル密度の決定（グローバルと流体を合成）
    float particleDensity = max(finalGlobalDensity, finalFluidDensity);

    // ---------------------------------------------------------
    // セルフシャドウ用密度の計算
    // ---------------------------------------------------------
    // グローバルフォグと流体を分けてから合成
    float finalGlobalShadowDensity = globalBaseDensity * combinedNoiseEffect;
    float finalFluidShadowDensity = fluidShadowMass; 
    
    float dynamicShadowFog = max(finalGlobalShadowDensity, finalFluidShadowDensity);

    // 光の減衰とライティング計算
    float fluidSelfShadow = exp(-dynamicShadowFog * 4.0f);

    // 受光量の計算（地形シャドウ × 流体セルフシャドウ を合成）
    float finalShadowVisibility = shadowVisibility * fluidSelfShadow;
    
    // 太陽と環境光の分離
    float cosTheta = dot(rayDir, normalize(-gFrameData.mainLightDirection));
    
    // 主光源（ゴッドレイ）のフェーズ関数。前方散乱を強くする
    float phase = DualPhaseHG(cosTheta, gFogSettings.anisotropy);
    
    float directionalScatteringMultiplier = gFrameData.mainLightVolumetricScatteringIntensity;

    // 太陽からの直接光
    float3 mainLightContrib = finalShadowVisibility * phase * gFrameData.mainLightColor.rgb * directionalScatteringMultiplier;

    // 環境光
    float3 ambientContrib = gFogSettings.ambientLight * lerp(0.3f, 1.0f, finalShadowVisibility);

    // ローカルライトを含める前のベースライト
    float3 totalLight = mainLightContrib + ambientContrib;

    // ローカルライトの累積用
    float3 stepLocal = 0;

    // ローカルライト用の煙による減衰
    float localFogAttenuation = exp(-particleDensity * 2.0f);

    for (int p = 0; p < MAX_POINT_LIGHTS; ++p)
    {
        if (gPointLights[p].enable == 0)
            continue;
      
        float3 lightVec = gPointLights[p].position - currentPos;
        float distance = length(lightVec);
        float radius = gPointLights[p].radius;
      
        // ボクセルの厚みに応じて距離測定をスムージング
        float smoothDistance = sqrt(distance * distance + voxelThickness * voxelThickness * 0.25f);

        // ハードカット(continue)の判定にもボクセル的なバッファ(1.1倍)を持たせる
        if (smoothDistance > radius * 1.1f)
            continue;
      
        float3 lightDir = (distance > 0.001f) ? (lightVec / distance) : float3(0.0f, 1.0f, 0.0f);
        
        // 境界をスパッと切らず、smoothstepで外周を滑らかにゼロへフェードアウト
        float baseAttenuate = pow(saturate(1.0f - smoothDistance / radius), 2.0f);
        float edgeFadeOut = smoothstep(radius * 1.1f, radius * 0.95f, smoothDistance);
        float attenuation = baseAttenuate * edgeFadeOut;

        float phaseLocal = DualPhaseHG(dot(rayDir, lightDir), gFogSettings.anisotropy);
        float pointLocalFogAttenuation = exp(-particleDensity * 1.0f);
        float volumetricScatteringIntensity = gPointLights[p].volumetricScatteringIntensity;

        stepLocal += gPointLights[p].color.rgb * (gPointLights[p].intensity * volumetricScatteringIntensity)
                  * attenuation * phaseLocal * pointLocalFogAttenuation;
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
        
        float3 lDir = lightVec / max(distance, 0.001f);
        if (distance < 0.2f)
        {
            float blend = smoothstep(0.0f, 0.2f, distance);
            lDir = normalize(lerp(-gSpotLights[s].direction, lDir, blend));
        }
        
        float currentCos = dot(-lDir, normalize(gSpotLights[s].direction));
        float cosOuter = gSpotLights[s].cosAngle;
        
        // コーン境界の判定。
        float voxelSmoothing = (voxelThickness / max(distance, 1.0f)) * 0.15f;
        
        // 完全にコーンの外側なら早期スキップ
        if (currentCos < cosOuter - voxelSmoothing)
            continue;
        
        // 距離減衰
        float safeMaxDist = max(gSpotLights[s].distance, 0.0001f);
        float distanceRatio = saturate(smoothDistance / safeMaxDist);
        float distanceAtt = pow(saturate(1.0f - distanceRatio), 2.0f);
        
        // 角度減衰
        float cosInner = min(cosOuter + 0.02f + voxelSmoothing, 1.0f);
        
        // 中心の芯が強く残り、輪郭もパキッとする
        float rawAngleAtt = saturate((currentCos - (cosOuter - voxelSmoothing)) / max(cosInner - (cosOuter - voxelSmoothing), 0.0001f));
        float angleAtt = rawAngleAtt * rawAngleAtt;
        
        float attenuation = distanceAtt * angleAtt;
        
        if (attenuation <= 0.0f)
            continue;
        
        // フェーズ関数（光の散乱方向）
        float phaseSmoothing = saturate(1.0f - (voxelThickness / max(distance, 0.5f)));
        float blurredAnisotropy = gFogSettings.anisotropy * phaseSmoothing;
        float phaseLocal = DualPhaseHG(dot(rayDir, lDir), blurredAnisotropy);
        
        float volumetricScatteringIntensity = gSpotLights[s].volumetricScatteringIntensity;
        float spotLocalFogAttenuation = exp(-particleDensity * 1.0f);

        // 最終合成
        stepLocal += gSpotLights[s].color.rgb * (gSpotLights[s].intensity * volumetricScatteringIntensity)
                   * attenuation * phaseLocal * spotLocalFogAttenuation;
    }

    totalLight += stepLocal;
    
    // 距離フェードの適用
    float fadeStart = farZ * 0.8f;
    float distanceFade = saturate((farZ - sampleViewZ) / max(farZ - fadeStart, 0.001f));
    particleDensity *= distanceFade;
    
    // フォグの総合的な濃さ（光を遮る強さ）を計算
    float global_sigma_e = max(particleDensity * gFogSettings.extinctionScale, 0.00001f);
    
    // その濃さのうち、アルベドの割合だけが光を散乱させる
    float3 global_sigma_s = global_sigma_e * gFogSettings.albedo;
    
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
            float3 volWarp = gNoiseVolume.SampleLevel(gSampler, volWarpUVW, 0.0f).rgb * 2.0f - 1.0f;
            float3 volNoisePos = (baseVolPos * vol.noiseScale) + volTimeOffset + (volWarp * vol.distortionAmount);

            float4 volNoiseSample = gNoiseVolume.SampleLevel(gSampler, volNoisePos, 0.0f);
            float volBase = volNoiseSample.r;
            float volCoarseErode = volNoiseSample.g;
            float volFineErode = volNoiseSample.b;
            
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

   // 不透明オブジェクトによるディープシールド（深度ウェイト）の簡易適用
    float fadeRange = max(voxelThickness * 1.0f, 0.1f);
    float depthWeight = saturate((sceneDist - sampleViewZ) / fadeRange);
    
    // Near Fade (手前フェード)
    // カメラの直前にあるボクセルを強制的に透明に
    float nearFadeStart = 0.5f; // フェード開始（0.5mまでは完全に透明）
    float nearFadeEnd = 3.0f; // フェード終了（3.0mで通常の濃さに）
    float nearFade = smoothstep(nearFadeStart, nearFadeEnd, sampleViewZ);
    
    // Near Fade を全体のウェイトに乗算する
    depthWeight *= nearFade;

    // 最終的な散乱光と消散係数の算出
    float3 scattering = ((totalLight * global_sigma_s) + volumeScattering) * depthWeight;
    float extinction = (global_sigma_e + volumeExtinction) * depthWeight; 

    // UAVへ書き込み
    gVoxelInject[DTid.xyz] = float4(scattering, extinction);
}