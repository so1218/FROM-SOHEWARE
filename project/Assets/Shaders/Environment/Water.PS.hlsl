#include "Common/Object3D.hlsli"
#include "Common/ShaderConstants.hlsli"
#include "Common/LightingUtils.hlsli"
#include "Common/PBRUtils.hlsli"
#include "Common/CameraUtils.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);

cbuffer DirectionalLights : register(b1)
{
    DirectionalLight gDirectionalLights[MAX_DIRECTIONAL_LIGHTS];
};
cbuffer PointLights : register(b2)
{
    PointLight gPointLights[MAX_POINT_LIGHTS];
};
cbuffer SpotLights : register(b3)
{
    SpotLight gSpotLights[MAX_SPOT_LIGHTS];
};

ConstantBuffer<WaterMaterialData> gWaterMaterial : register(b5);
ConstantBuffer<InteractionConstants> gInteractionData : register(b8);

Texture2D<float4> gSceneColorTexture : register(t0);
Texture2D<float> gSceneDepthTexture : register(t1);
TextureCube<float4> gEnvironmentTexture : register(t2);
Texture2D<float4> gWaterNormalMap : register(t3);
Texture2D<float4> gInteractionMap : register(t11);

SamplerState gSampler : register(s0);
SamplerState gClampSampler : register(s1);

struct WaterPSInput
{
    float4 position : SV_POSITION;
    float3 worldPosition : POSITION1;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
    float3 tangent : TANGENT;
    float4 worldColor : COLOR0;
    float4 currentClipPos : POSITION2;
};

struct WaterPSOutput
{
    float4 color : SV_TARGET0;
    float4 normal : SV_TARGET1;
    float4 material : SV_TARGET2;
};

float3 BlendNormalsRNM(float3 baseNormal, float3 detailNormal)
{
    float3 t = baseNormal + float3(0.0f, 0.0f, 1.0f);
    float3 u = detailNormal * float3(-1.0f, -1.0f, 1.0f);
    return t * dot(t, u) / max(t.z, kEpsilon) - u;
}

// ---------------------------------------------------------
// SSR
// ---------------------------------------------------------
float3 TraceSSR(float3 rayOrigin, float3 smoothReflectDir, float3 worldNormal, float roughness, float2 screenUV, out float hitWeight)
{
    hitWeight = 0.0f;

    float2 pixelPos = screenUV * gFrameData.screenResolution.xy;
    float dither = InterleavedGradientNoise(pixelPos + (gFrameData.gTime * 144.0f));

    // CPU側からのパラメータ取得 (安全のためのクランプ処理)
    int maxSteps = (int) clamp(gWaterMaterial.ssrMaxSteps, 8.0f, 128.0f);
    int binarySearchSteps = (int) clamp(gWaterMaterial.ssrBinarySearchSteps, 0.0f, 16.0f);

    float stepSize = max(gWaterMaterial.ssrStepSize, 0.05f);
    float maxDist = max(gWaterMaterial.ssrMaxDistance, 10.0f);

    float3 rayPos = rayOrigin + smoothReflectDir * (stepSize * dither);
    float3 lastRayPos = rayPos;
    float traveled = 0.0f;

    bool hit = false;
    float2 hitUV = 0.0f;

    float4 projStart = mul(float4(rayOrigin, 1.0f), gFrameData.viewProjectionMatrix);
    float4 projDir = mul(float4(smoothReflectDir, 0.0f), gFrameData.viewProjectionMatrix);
    
    // 動的な maxSteps でループを打ち切る
    [loop]
    for (int i = 0; i < maxSteps; ++i)
    {
        if (traveled > maxDist)
            break;

        rayPos += smoothReflectDir * stepSize;
        traveled += stepSize;
        stepSize *= 1.025f;

        float4 projPos = projStart + projDir * traveled;
        
        if (projPos.w <= kEpsilon)
            continue;

        float3 ndc = projPos.xyz / projPos.w;
        float2 uv = ndc.xy * float2(0.5f, -0.5f) + 0.5f;

        if (any(uv < 0.0f) || any(uv > 1.0f))
            break;

        float sceneRawDepth = gSceneDepthTexture.SampleLevel(gClampSampler, uv, 0).r;
        float sceneLinearDepth = LinearizeDepth(sceneRawDepth, gFrameData.nearClip, gFrameData.farClip);
        float rayLinearDepth = LinearizeDepth(ndc.z, gFrameData.nearClip, gFrameData.farClip);
        float depthDiff = rayLinearDepth - sceneLinearDepth;

        float dynamicThickness = gWaterMaterial.ssrThickness * max(1.0f, rayLinearDepth * 0.05f);

        if (depthDiff > 0.0f && depthDiff < dynamicThickness)
        {
            hit = true;
            break;
        }
        lastRayPos = rayPos;
    }

    // 二分探索
    if (hit)
    {
        float3 minPos = lastRayPos;
        float3 maxPos = rayPos;
        float3 midPos = minPos;

        [loop]
        for (int j = 0; j < binarySearchSteps; ++j)
        {
            midPos = lerp(minPos, maxPos, 0.5f);
            
            float4 projPos = mul(float4(midPos, 1.0f), gFrameData.viewProjectionMatrix);
            
            float clipW = max(projPos.w, kEpsilon);
            float3 ndc = projPos.xyz / clipW;
            float2 uv = ndc.xy * float2(0.5f, -0.5f) + 0.5f;

            float sceneLinearDepth = LinearizeDepth(gSceneDepthTexture.SampleLevel(gClampSampler, uv, 0).r, gFrameData.nearClip, gFrameData.farClip);
            float rayLinearDepth = LinearizeDepth(ndc.z, gFrameData.nearClip, gFrameData.farClip);
            float depthDiff = rayLinearDepth - sceneLinearDepth;

            float dynamicThickness = gWaterMaterial.ssrThickness * max(1.0f, rayLinearDepth * 0.05f);

            if (depthDiff > 0.0f && depthDiff < dynamicThickness)
            {
                maxPos = midPos;
                hitUV = uv;
            }
            else
            {
                minPos = midPos;
            }
        }

        // 画面端およびレイ長に応じたブレンド係数の算出
        float2 edgeFade2 = smoothstep(0.0f, 0.05f, hitUV) * smoothstep(1.0f, 0.95f, hitUV);
        float screenEdgeFade = edgeFade2.x * edgeFade2.y;
        float rayDistance = distance(rayOrigin, midPos);
        float rayLengthFade = 1.0f - smoothstep(maxDist * 0.5f, maxDist, rayDistance);

        hitWeight = screenEdgeFade * rayLengthFade;
        
        float distFactor = saturate(rayDistance / maxDist);
        float2 distortionOffset = worldNormal.xz * float2(1.0f, -1.0f) * gWaterMaterial.ssrDistortion * (1.0f - distFactor * 0.5f);
        float2 distortedUV = clamp(hitUV + distortionOffset, 0.005f, 0.995f);
        float mipLevel = roughness * 8.0f + (distFactor * 3.0f);

        return gSceneColorTexture.SampleLevel(gClampSampler, distortedUV, mipLevel).rgb;
    }

    return float3(0.0f, 0.0f, 0.0f);
}

WaterPSOutput main(WaterPSInput input)
{
    WaterPSOutput output;

    // 視線ベクトルの算出
    float3 V = normalize(gFrameData.cameraWorldPosition - input.worldPosition);
    
    // スクリーン空間UVの算出
    float clipW = max(input.currentClipPos.w, kEpsilon);
    float2 screenUV = (input.currentClipPos.xy / clipW) * float2(0.5f, -0.5f) + 0.5f;

    // 水深の計算（シーン深度と水面深度の差分）
    float sceneRawDepth = gSceneDepthTexture.Sample(gClampSampler, screenUV).r;
    float sceneLinearDepth = LinearizeDepth(sceneRawDepth, gFrameData.nearClip, gFrameData.farClip);
    float waterLinearDepth = LinearizeDepth(input.currentClipPos.z / clipW, gFrameData.nearClip, gFrameData.farClip);
    float waterDepth = max(sceneLinearDepth - waterLinearDepth, 0.0f);

    float time = gFrameData.gTime * max(gWaterMaterial.waveSpeed, 0.1f);
    
    // 風向ベクトルの正規化
    float2 windDir = gWaterMaterial.globalWindDirection;
    float windLen = length(windDir);
    windDir = (windLen > kEpsilon) ? (windDir / windLen) : float2(1.0f, 0.0f);

    // =========================================================
    // 波法線の合成
    // =========================================================
    float2 uv1 = input.worldPosition.xz * gWaterMaterial.normalTiling.x - windDir * (time * 0.08f);
    float2 windDir2 = float2(windDir.x * 0.866f - windDir.y * 0.5f, windDir.x * 0.5f + windDir.y * 0.866f);
    float2 uv2 = input.worldPosition.xz * gWaterMaterial.normalTiling.y - windDir2 * (time * 0.04f);

    float3 n1 = gWaterNormalMap.Sample(gSampler, uv1).rgb * 2.0f - 1.0f;
    float3 n2 = gWaterNormalMap.Sample(gSampler, uv2).rgb * 2.0f - 1.0f;
    
    n1.xy *= gWaterMaterial.normalIntensity;
    n2.xy *= gWaterMaterial.normalIntensity;
    n1.z = sqrt(saturate(1.0f - dot(n1.xy, n1.xy)));
    n2.z = sqrt(saturate(1.0f - dot(n2.xy, n2.xy)));

    float3 finalTangentNormal = normalize(BlendNormalsRNM(n1, n2));

    // =========================================================
    // プレイヤー・オブジェクト干渉法線の合成
    // =========================================================
    float2 interactUV = (input.worldPosition.xz - gInteractionData.centerWorldPos) / gInteractionData.worldSize + 0.5f;
    float4 interactData = float4(0.5f, 0.5f, 0.0f, 0.0f);

    if (all(interactUV >= 0.0f) && all(interactUV <= 1.0f))
    {
        interactData = gInteractionMap.Sample(gClampSampler, interactUV);
    }

    float2 pushDirXZ = (interactData.rg * 2.0f) - 1.0f;
    float interactTrail = interactData.a;

    float3 interactNormal = normalize(float3(pushDirXZ * interactTrail * gWaterMaterial.interactionNormalScale, 1.0f));
    finalTangentNormal = normalize(BlendNormalsRNM(finalTangentNormal, interactNormal));
    
    // 接空間(TBN)からワールド空間へ変換
    float3 N = normalize(input.normal);
    float3 T = normalize(input.tangent);
    float3 B = normalize(cross(N, T));
    float3x3 TBN = float3x3(T, B, N);
    float3 worldNormal = normalize(mul(finalTangentNormal, TBN));

    worldNormal.y = max(worldNormal.y, 0.15f);
    worldNormal = normalize(worldNormal);

    // =========================================================
    // 水中表示（屈折・色収差）
    // =========================================================
    float distortion = smoothstep(0.0f, 0.5f, waterDepth) * gWaterMaterial.refractionAmount;
    float caOffset = gWaterMaterial.chromaticAberration * 0.01f;

    float2 refractUV_R = screenUV + finalTangentNormal.xy * (distortion + caOffset);
    float2 refractUV_G = screenUV + finalTangentNormal.xy * distortion;
    float2 refractUV_B = screenUV + finalTangentNormal.xy * (distortion - caOffset);

    if (LinearizeDepth(gSceneDepthTexture.Sample(gClampSampler, refractUV_G).r, gFrameData.nearClip, gFrameData.farClip) < waterLinearDepth)
    {
        refractUV_R = refractUV_G = refractUV_B = screenUV;
    }

    float3 sceneColor;
    sceneColor.r = gSceneColorTexture.Sample(gClampSampler, refractUV_R).r;
    sceneColor.g = gSceneColorTexture.Sample(gClampSampler, refractUV_G).g;
    sceneColor.b = gSceneColorTexture.Sample(gClampSampler, refractUV_B).b;
    float3 lightDir = normalize(-gDirectionalLights[0].direction);

    // Beer-Lambertの法則に基づく水深による吸光処理
    float transmittance = exp(-waterDepth * gWaterMaterial.absorption);
    float3 waterBodyColor = lerp(gWaterMaterial.deepColor.rgb, gWaterMaterial.shallowColor.rgb, transmittance);
    
    // 水中散乱（Subsurface Scatteringの擬似表現）
    float waveSubsurface = saturate(dot(V, -lightDir) + 0.5f) * (interactTrail * 0.8f + (1.0f - finalTangentNormal.z) * 0.5f);
    float scatterFactor = pow(saturate(dot(V, -lightDir)), 4.0f) * (1.0f - transmittance);
    float3 inScattering = gWaterMaterial.scatterColor.rgb * (scatterFactor + waveSubsurface * 1.5f);

    // コースティクスなしの水中光計算
    float3 refractedLight = sceneColor * waterBodyColor + inScattering;

    // =========================================================
    // 水面反射（SSR / キューブマップ・フレネル合成）
    // =========================================================
    float NdotV = saturate(dot(worldNormal, V));
    float fresnel = 0.04f + (1.0f - 0.04f) * pow(1.0f - NdotV, 5.0f);

    float3 reflectVector = reflect(-V, worldNormal);
    reflectVector.y = max(reflectVector.y, 0.02f);
    reflectVector = normalize(reflectVector);

    float3 skyReflection = gEnvironmentTexture.SampleLevel(gSampler, reflectVector, gWaterMaterial.roughness * 5.0f).rgb;
    skyReflection *= gWaterMaterial.envReflectionIntensity;

    float3 smoothReflectVector = reflect(-V, N);
    smoothReflectVector.y = max(smoothReflectVector.y, 0.02f);
    smoothReflectVector = normalize(smoothReflectVector);

    float ssrWeight = 0.0f;
    float3 ssrColor = TraceSSR(input.worldPosition, smoothReflectVector, worldNormal, gWaterMaterial.roughness, screenUV, ssrWeight);

    float3 finalReflection = lerp(skyReflection, ssrColor, ssrWeight * gWaterMaterial.ssrIntensity);
    float edgeFade = smoothstep(0.0f, 0.1f, waterDepth);
    float3 finalColor = lerp(refractedLight, finalReflection, fresnel * edgeFade);

    // =========================================================
    // 泡の生成とブレンド
    // =========================================================
    float shoreMask = 1.0f - saturate(waterDepth / max(gWaterMaterial.shoreFoamThreshold, kEpsilon));
    float waveSlope = 1.0f - finalTangentNormal.z;
    float wavePeakMask = smoothstep(gWaterMaterial.waveFoamThreshold, 1.0f, waveSlope);
    float playerFoamMask = saturate(interactTrail * gWaterMaterial.interactionFoamIntensity);

    float foamMask = saturate(max(max(shoreMask, wavePeakMask * 0.8f), playerFoamMask));

    if (foamMask > 0.01f)
    {
        float2 foamUV1 = input.worldPosition.xz * gWaterMaterial.foamScale + windDir * (time * 0.05f);
        float2 foamUV2 = input.worldPosition.xz * (gWaterMaterial.foamScale * 1.3f) - windDir * (time * 0.03f);

        float foamNoise1 = gWaterNormalMap.Sample(gSampler, foamUV1).r;
        float foamNoise2 = gWaterNormalMap.Sample(gSampler, foamUV2).r;
        float organicFoamPattern = saturate((foamNoise1 + foamNoise2) * 0.75f);

        float foamCutoff = 1.0f - foamMask;
        float finalFoamIntensity = smoothstep(foamCutoff, foamCutoff + 0.25f, organicFoamPattern);

        finalColor = lerp(finalColor, gWaterMaterial.foamColor.rgb, finalFoamIntensity * gWaterMaterial.foamIntensity * gWaterMaterial.foamColor.a);
    }

    // =========================================================
    // スペキュラハイライト & G-Buffer出力設定
    // =========================================================
    float3 H = normalize(lightDir + V);
    float spec = pow(saturate(dot(worldNormal, H)), 256.0f / max(gWaterMaterial.roughness, kEpsilon));
    finalColor += gDirectionalLights[0].color.rgb * spec * gWaterMaterial.specularIntensity * edgeFade;

    output.color = float4(finalColor, 1.0f);
    output.normal = float4(worldNormal, 1.0f);
    output.material = float4(gWaterMaterial.roughness, 0.0f, 0.0f, 1.0f);
    
    return output;
}