//#include "FullScreenQuad.hlsli"
//#include "ShaderConstants.hlsli"

//Texture2D<float> gDepthTexture : register(t0); 
//Texture2D<float> gShadowMap : register(t1);

//SamplerState gSampler : register(s0);
//SamplerComparisonState gShadowSampler : register(s1); 

//ConstantBuffer<FrameData> gFrameData : register(b0);
//ConstantBuffer<VolumetricFogSettings> gFogSettings : register(b2);

//float PhaseFunctionHG(float cosTheta, float g)
//{
//    float g2 = g * g;
//    float denom = 1.0f + g2 - 2.0f * g * cosTheta;
//    return (1.0f - g2) / (4.0f * 3.14159265f * pow(denom, 1.5f));
//}

//float DualPhaseHG(float cosTheta, float g)
//{

//    float forward = PhaseFunctionHG(cosTheta, g);

//    float backward = PhaseFunctionHG(cosTheta, -0.2f); 
    
//    return lerp(backward, forward, 0.9f);
//}

//float SimpleCloudNoise(float3 p)
//{
//    float n = sin(p.x) * sin(p.y) * sin(p.z);
//    n += sin(p.x * 2.2f + 1.1f) * sin(p.y * 2.3f + 2.2f) * sin(p.z * 2.4f + 3.3f) * 0.5f;
//    return saturate(n * 0.5f + 0.5f);
//}

//float4 main(VSOutput input) : SV_TARGET
//{
//    float depthVal = gDepthTexture.Sample(gSampler, input.uv);

//    // ワールド座標を復元
//    float clipX = input.uv.x * 2.0f - 1.0f;
//    float clipY = (1.0f - input.uv.y) * 2.0f - 1.0f;
//    float4 clipPos = float4(clipX, clipY, depthVal, 1.0f);
//    float4 worldPos = mul(clipPos, gFrameData.invViewProj);
//    worldPos /= worldPos.w;

//    // レイマーチングの準備
//    float3 rayVec = worldPos.xyz - gFrameData.cameraWorldPosition;
//    float rayLength = length(rayVec);
//    float3 rayDir = rayVec / max(rayLength, 0.0001f);

//    // 最大距離とステップ数をパラメータから取得
//    float marchLength = min(rayLength, gFogSettings.maxDistance);
//    int steps = gFogSettings.steps;
//    float stepSize = marchLength / max((float) steps, 1.0f); // 0割り防止
    
//    // ディザリング
//    float dither = frac(sin(dot(input.uv, float2(12.9898, 78.233))) * 43758.5453) * stepSize;
//    float3 currentPos = gFrameData.cameraWorldPosition + (rayDir * dither);

//    float3 volumetricIllumination = float3(0, 0, 0);
//    // 光の方向ベクトルを反転させて太陽の方向に向ける
//    float3 lightDir = normalize(-gFrameData.mainLightDirection);
//    float cosTheta = dot(rayDir, lightDir);

//    float phase = DualPhaseHG(cosTheta, gFogSettings.scatteringG);
    
//    // ループ開始前の準備
//    float transmittance = 1.0f;
//    float3 ambientLight = float3(0.05f, 0.05f, 0.07f); 

//    // レイマーチング・ループ
//    for (int i = 0; i < steps; ++i)
//    {
//        float4 shadowCoord = mul(float4(currentPos, 1.0f), gFrameData.lightViewProj);
//        shadowCoord.xyz /= shadowCoord.w;
//        float2 shadowUV = shadowCoord.xy * float2(0.5f, -0.5f) + 0.5f;
//        float shadowVisibility = 1.0f;
//        if (shadowUV.x >= 0.0f && shadowUV.x <= 1.0f && shadowUV.y >= 0.0f && shadowUV.y <= 1.0f && shadowCoord.z >= 0.0f && shadowCoord.z <= 1.0f)
//        {
//            float compareDepth = shadowCoord.z - 0.001f;
//            shadowVisibility = gShadowMap.SampleCmpLevelZero(gShadowSampler, shadowUV, compareDepth);
//        }

//        float heightFalloff = exp(-max(currentPos.y - gFogSettings.baseHeight, 0.0f) * gFogSettings.heightFalloff);
//        float noiseVal = SimpleCloudNoise(currentPos * gFogSettings.noiseScale + (float3(gFrameData.gTime * gFogSettings.windSpeed, 0, 0)));
//        noiseVal = smoothstep(gFogSettings.noiseThreshold, 1.0f, noiseVal);
//        float stepDensity = gFogSettings.density * heightFalloff * noiseVal;

//        float stepAttenuation = exp(-stepDensity * stepSize);

//        float3 directLight = shadowVisibility * phase * gFrameData.mainLightColor.rgb;

//        float ambientOcclusion = lerp(0.4f, 1.0f, shadowVisibility);
//        float3 ambientColor = gFogSettings.fogColor * gFogSettings.ambientFactor * ambientOcclusion;

//        float3 scatteringLight = (directLight * gFogSettings.fogColor + ambientColor);
    
//        float3 stepScattering = scatteringLight * (1.0f - stepAttenuation);

//    // 現在の透過率を掛け合わせて加算
//        volumetricIllumination += stepScattering * transmittance;

//    // 透過率を更新
//        transmittance *= stepAttenuation;

//        currentPos += rayDir * stepSize;
//    }
    
//    volumetricIllumination *= gFogSettings.intensity;

//    return float4(volumetricIllumination, transmittance);
//}