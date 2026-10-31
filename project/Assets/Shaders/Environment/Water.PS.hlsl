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

ConstantBuffer<WaterMaterialData> gWaterMaterial : register(b5);
ConstantBuffer<InteractionConstants> gInteractionData : register(b8);

Texture2D<float4> gSceneColorTexture : register(t0);   // 水より先に描いた不透明物の色
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
};

// 2 枚の法線マップを、凹凸を打ち消し合わずに重ねる (Reoriented Normal Mapping)
float3 BlendNormals(float3 baseNormal, float3 detailNormal)
{
    float3 shiftedBase = baseNormal + float3(0.0f, 0.0f, 1.0f);
    float3 flippedDetail = detailNormal * float3(-1.0f, -1.0f, 1.0f);
    return shiftedBase * dot(shiftedBase, flippedDetail) / max(shiftedBase.z, kEpsilon) - flippedDetail;
}

// 法線マップを読み、xy を強めてから z を作り直す
float3 SampleWaterNormal(float2 uv)
{
    float3 normal = gWaterNormalMap.Sample(gSampler, uv).rgb * 2.0f - 1.0f;
    normal.xy *= gWaterMaterial.normalIntensity;
    normal.z = sqrt(saturate(1.0f - dot(normal.xy, normal.xy)));
    return normal;
}

float LinearDepthAt(float2 screenUV)
{
    return LinearizeDepth(gSceneDepthTexture.SampleLevel(gClampSampler, screenUV, 0).r, gFrameData.nearClip, gFrameData.farClip);
}

// レイがシーンの表面のすぐ後ろにあるか。遠いほど判定の厚みを広げる
bool IsBehindSurface(float rayLinearDepth, float sceneLinearDepth)
{
    float depthDifference = rayLinearDepth - sceneLinearDepth;
    float thickness = gWaterMaterial.ssrThickness * max(1.0f, rayLinearDepth * 0.05f);
    return depthDifference > 0.0f && depthDifference < thickness;
}

// 画面に映っているものの反射 (SSR)。当たらなければ hitWeight = 0
float3 TraceScreenSpaceReflection(float3 rayOrigin, float3 rayDirection, float3 surfaceNormal, float2 screenUV, out float hitWeight)
{
    hitWeight = 0.0f;

    int maxSteps = (int) clamp(gWaterMaterial.ssrMaxSteps, 8.0f, 128.0f);
    int refineSteps = (int) clamp(gWaterMaterial.ssrBinarySearchSteps, 0.0f, 16.0f);
    float stepSize = max(gWaterMaterial.ssrStepSize, 0.05f);
    float maxDistance = max(gWaterMaterial.ssrMaxDistance, 10.0f);

    // 開始位置をピクセルごとにずらして、段差の縞を目立たなくする
    float dither = InterleavedGradientNoise(screenUV * gFrameData.screenResolution.xy + gFrameData.gTime * 144.0f);
    float traveled = stepSize * dither;
    float previousTraveled = traveled;

    bool isHit = false;
    float2 hitUV = float2(0.0f, 0.0f);

    // 少しずつ進めて、シーンの表面の後ろに入った所を探す (遠いほど歩幅を広げる)
    [loop]
    for (int stepIndex = 0; stepIndex < maxSteps && traveled <= maxDistance; ++stepIndex)
    {
        previousTraveled = traveled;
        traveled += stepSize;
        stepSize *= 1.025f;

        float4 clipPosition = mul(float4(rayOrigin + rayDirection * traveled, 1.0f), gFrameData.viewProjectionMatrix);
        if (clipPosition.w <= kEpsilon)
        {
            continue;
        }

        float3 ndc = clipPosition.xyz / clipPosition.w;
        float2 uv = ndc.xy * float2(0.5f, -0.5f) + 0.5f;
        if (any(uv < 0.0f) || any(uv > 1.0f))
        {
            break;
        }

        if (IsBehindSurface(LinearizeDepth(ndc.z, gFrameData.nearClip, gFrameData.farClip), LinearDepthAt(uv)))
        {
            isHit = true;
            hitUV = uv;
            break;
        }
    }

    if (!isHit)
    {
        return float3(0.0f, 0.0f, 0.0f);
    }

    // 当たった区間を二分探索して、位置を詰める
    float nearDistance = previousTraveled;
    float farDistance = traveled;
    [loop]
    for (int refineIndex = 0; refineIndex < refineSteps; ++refineIndex)
    {
        float middleDistance = (nearDistance + farDistance) * 0.5f;
        float4 clipPosition = mul(float4(rayOrigin + rayDirection * middleDistance, 1.0f), gFrameData.viewProjectionMatrix);
        float3 ndc = clipPosition.xyz / max(clipPosition.w, kEpsilon);
        float2 uv = ndc.xy * float2(0.5f, -0.5f) + 0.5f;

        if (IsBehindSurface(LinearizeDepth(ndc.z, gFrameData.nearClip, gFrameData.farClip), LinearDepthAt(uv)))
        {
            farDistance = middleDistance;
            hitUV = uv;
        }
        else
        {
            nearDistance = middleDistance;
        }
    }

    // 画面の端と、遠い反射は弱める
    float2 screenEdgeFade = smoothstep(0.0f, 0.05f, hitUV) * smoothstep(1.0f, 0.95f, hitUV);
    float distanceRatio = saturate(farDistance / maxDistance);
    hitWeight = screenEdgeFade.x * screenEdgeFade.y * (1.0f - smoothstep(0.5f, 1.0f, distanceRatio));

    // 波で反射をゆがませ、遠いほどぼかす
    float2 distortion = surfaceNormal.xz * float2(1.0f, -1.0f) * gWaterMaterial.ssrDistortion * (1.0f - distanceRatio * 0.5f);
    float2 reflectionUV = clamp(hitUV + distortion, 0.005f, 0.995f);
    float mipLevel = gWaterMaterial.roughness * 8.0f + distanceRatio * 3.0f;
    return gSceneColorTexture.SampleLevel(gClampSampler, reflectionUV, mipLevel).rgb;
}

PixelShaderOutput main(WaterPSInput input)
{
    float3 toCamera = normalize(gFrameData.cameraWorldPosition - input.worldPosition);
    float3 lightDirection = normalize(-gDirectionalLights[0].direction);
    float2 screenUV = input.position.xy / gFrameData.screenResolution.xy;

    // 水の深さ = 水面から、その奥にある不透明物までの距離
    float waterSurfaceDepth = LinearizeDepth(input.position.z, gFrameData.nearClip, gFrameData.farClip);
    float waterDepth = max(LinearDepthAt(screenUV) - waterSurfaceDepth, 0.0f);

    float time = gFrameData.gTime * max(gWaterMaterial.waveSpeed, 0.1f);
    float windLength = length(gWaterMaterial.globalWindDirection);
    float2 windDirection = (windLength > kEpsilon) ? gWaterMaterial.globalWindDirection / windLength : float2(1.0f, 0.0f);

    // 細かい波: 法線マップを 2 枚、大きさと向き (30 度ずらす) を変えて流して重ねる
    float2 rotatedWind = float2(windDirection.x * 0.866f - windDirection.y * 0.5f, windDirection.x * 0.5f + windDirection.y * 0.866f);
    float3 largeRipple = SampleWaterNormal(input.worldPosition.xz * gWaterMaterial.normalTiling.x - windDirection * (time * 0.08f));
    float3 smallRipple = SampleWaterNormal(input.worldPosition.xz * gWaterMaterial.normalTiling.y - rotatedWind * (time * 0.04f));
    float3 tangentNormal = normalize(BlendNormals(largeRipple, smallRipple));

    // プレイヤーなどが通った跡の波紋 (rg = 押された向き, a = 通った跡)
    float2 interactionUV = (input.worldPosition.xz - gInteractionData.centerWorldPos) / gInteractionData.worldSize + 0.5f;
    float4 interaction = float4(0.5f, 0.5f, 0.0f, 0.0f);
    if (all(interactionUV >= 0.0f) && all(interactionUV <= 1.0f))
    {
        interaction = gInteractionMap.Sample(gClampSampler, interactionUV);
    }
    float trail = interaction.a;
    float3 trailNormal = normalize(float3((interaction.rg * 2.0f - 1.0f) * trail * gWaterMaterial.interactionNormalScale, 1.0f));
    tangentNormal = normalize(BlendNormals(tangentNormal, trailNormal));

    // 接空間 → ワールド空間
    float3 geometryNormal = normalize(input.normal);
    float3 tangent = normalize(input.tangent - geometryNormal * dot(input.tangent, geometryNormal));
    float3 bitangent = cross(geometryNormal, tangent);
    float3 normal = normalize(mul(tangentNormal, float3x3(tangent, bitangent, geometryNormal)));
    // 横を向きすぎて反射が破綻しないようにする
    normal.y = max(normal.y, 0.15f);
    normal = normalize(normal);

    // 水の中: 波でゆがめた奥の景色。色ごとに少しずらして色収差を出す
    float refraction = smoothstep(0.0f, 0.5f, waterDepth) * gWaterMaterial.refractionAmount;
    float chromaticOffset = gWaterMaterial.chromaticAberration * 0.01f;
    float2 refractionUV = screenUV + tangentNormal.xy * refraction;
    float2 redOffset = tangentNormal.xy * chromaticOffset;

    // ずらした先が水より手前の物なら、ずらさない (手前の物が水に映り込まないように)
    if (LinearDepthAt(refractionUV) < waterSurfaceDepth)
    {
        refractionUV = screenUV;
        redOffset = float2(0.0f, 0.0f);
    }

    float3 sceneColor;
    sceneColor.r = gSceneColorTexture.Sample(gClampSampler, refractionUV + redOffset).r;
    sceneColor.g = gSceneColorTexture.Sample(gClampSampler, refractionUV).g;
    sceneColor.b = gSceneColorTexture.Sample(gClampSampler, refractionUV - redOffset).b;

    // 深いほど光が吸収されて、深い色になる
    float transmittance = exp(-waterDepth * gWaterMaterial.absorption);
    float3 waterBodyColor = lerp(gWaterMaterial.deepColor.rgb, gWaterMaterial.shallowColor.rgb, transmittance);

    // 逆光で波や通った跡が透けて光る
    float backLight = dot(toCamera, -lightDirection);
    float waveScatter = saturate(backLight + 0.5f) * (trail * 0.8f + (1.0f - tangentNormal.z) * 0.5f);
    float deepScatter = pow(saturate(backLight), 4.0f) * (1.0f - transmittance);
    float3 underwaterColor = sceneColor * waterBodyColor + gWaterMaterial.scatterColor.rgb * (deepScatter + waveScatter * 1.5f);

    // 水面の反射: 空 (キューブマップ) と、画面に映っているもの (SSR)
    float fresnel = 0.04f + 0.96f * pow(1.0f - saturate(dot(normal, toCamera)), 5.0f);

    float3 reflectDirection = reflect(-toCamera, normal);
    reflectDirection.y = max(reflectDirection.y, 0.02f);
    float3 skyReflection = gEnvironmentTexture.SampleLevel(gSampler, normalize(reflectDirection), gWaterMaterial.roughness * 5.0f).rgb
        * gWaterMaterial.envReflectionIntensity;

    // SSR は細かい波で散らばらないよう、元の面の法線で飛ばす
    float3 smoothReflectDirection = reflect(-toCamera, geometryNormal);
    smoothReflectDirection.y = max(smoothReflectDirection.y, 0.02f);
    float ssrWeight;
    float3 ssrColor = TraceScreenSpaceReflection(input.worldPosition, normalize(smoothReflectDirection), normal, screenUV, ssrWeight);

    float3 reflection = lerp(skyReflection, ssrColor, ssrWeight * gWaterMaterial.ssrIntensity);

    // 岸のごく浅い所は反射を弱めて、地面となじませる
    float shoreFade = smoothstep(0.0f, 0.1f, waterDepth);
    float3 color = lerp(underwaterColor, reflection, fresnel * shoreFade);

    // 泡: 岸の浅い所・波の傾いた所・通った跡に出す
    float shoreFoam = 1.0f - saturate(waterDepth / max(gWaterMaterial.shoreFoamThreshold, kEpsilon));
    float waveFoam = smoothstep(gWaterMaterial.waveFoamThreshold, 1.0f, 1.0f - tangentNormal.z) * 0.8f;
    float trailFoam = saturate(trail * gWaterMaterial.interactionFoamIntensity);
    float foamAmount = saturate(max(max(shoreFoam, waveFoam), trailFoam));

    if (foamAmount > 0.01f)
    {
        // 法線マップを 2 方向に流して、泡のまだら模様にする
        float2 foamUV = input.worldPosition.xz * gWaterMaterial.foamScale;
        float foamPattern = saturate((
            gWaterNormalMap.Sample(gSampler, foamUV + windDirection * (time * 0.05f)).r +
            gWaterNormalMap.Sample(gSampler, foamUV * 1.3f - windDirection * (time * 0.03f)).r) * 0.75f);

        float foamStart = 1.0f - foamAmount;
        float foam = smoothstep(foamStart, foamStart + 0.25f, foamPattern);
        color = lerp(color, gWaterMaterial.foamColor.rgb, foam * gWaterMaterial.foamIntensity * gWaterMaterial.foamColor.a);
    }

    // 太陽のハイライト
    float3 halfVector = normalize(lightDirection + toCamera);
    float specular = pow(saturate(dot(normal, halfVector)), 256.0f / max(gWaterMaterial.roughness, kEpsilon));
    color += gDirectionalLights[0].color.rgb * specular * gWaterMaterial.specularIntensity * shoreFade;

    PixelShaderOutput output;
    output.color = float4(color, 1.0f);
    output.normal = float4(normal, 1.0f);
    return output;
}
