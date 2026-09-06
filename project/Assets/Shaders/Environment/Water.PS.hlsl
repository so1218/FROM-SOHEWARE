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

Texture2D<float4> gSceneColorTexture : register(t0);
Texture2D<float> gSceneDepthTexture : register(t1);
TextureCube<float4> gEnvironmentTexture : register(t2);
Texture2D<float4> gWaterNormalMap : register(t3);
Texture2D<float4> gRippleTexture : register(t4);

SamplerState gSampler : register(s0);
SamplerState gClampSampler : register(s2);

struct WaterPSOutput
{
    float4 color : SV_TARGET0;
    float4 normal : SV_TARGET1;
    float4 material : SV_TARGET2;
    float2 velocity : SV_TARGET3;
};

float3 BlendNormalsRNM(float3 baseNormal, float3 detailNormal)
{
    float3 t = baseNormal + float3(0.0f, 0.0f, 1.0f);
    float3 u = detailNormal * float3(-1.0f, -1.0f, 1.0f);
    return t * dot(t, u) / max(t.z, kEpsilon) - u;
}

float ProceduralFoamNoise(float2 uv)
{
    float2 i = floor(uv);
    float2 f = frac(uv);
    f = f * f * (3.0f - 2.0f * f);

    float a = Hash12(i);
    float b = Hash12(i + float2(1.0f, 0.0f));
    float c = Hash12(i + float2(0.0f, 1.0f));
    float d = Hash12(i + float2(1.0f, 1.0f));

    return lerp(lerp(a, b, f.x), lerp(c, d, f.x), f.y);
}

float3 TraceSSR_HQ(float3 rayOrigin, float3 smoothReflectDir, float3 worldNormal, float roughness, float2 screenUV, out float hitWeight)
{
    hitWeight = 0.0f;

    float2 pixelPos = screenUV * gFrameData.screenResolution.xy;
    float dither = InterleavedGradientNoise(pixelPos + (gFrameData.gTime * 144.0f));

    // ★ ステップ数を最適化（品質と負荷のバランス）
    static const int MAX_STEPS = 80;
    static const int BINARY_SEARCH_STEPS = 8;

    float stepSize = max(gWaterMaterial.ssrStepSize, 0.05f);
    float maxDist = max(gWaterMaterial.ssrMaxDistance, 10.0f);

    // ★ 縞々対策 1: レイの開始位置をピクセルごとにランダムにずらす
    // 規則的な縞模様が「微細なノイズ」に変換され、自然に見えるようになります
    float3 rayPos = rayOrigin + smoothReflectDir * (stepSize * dither);
    float3 lastRayPos = rayPos;
    float traveled = 0.0f;

    bool hit = false;
    float2 hitUV = 0.0f;
    
    // 1. レイマーチング
    [unroll(MAX_STEPS)]
    for (int i = 0; i < MAX_STEPS; ++i)
    {
        if (traveled > maxDist)
            break;

        rayPos += smoothReflectDir * stepSize;
        traveled += stepSize;
        stepSize *= 1.025f;

        float4 projPos = mul(float4(rayPos, 1.0f), gFrameData.viewProjectionMatrix);
        // kEpsilon でカメラ背面・Nearクリップ付近を安全にスキップ
        if (projPos.w <= kEpsilon)
            continue;

        float3 ndc = projPos.xyz / projPos.w;
        float2 uv = ndc.xy * float2(0.5f, -0.5f) + 0.5f;

        if (any(uv < 0.0f) || any(uv > 1.0f))
            break;

        float sceneRawDepth = gSceneDepthTexture.SampleLevel(gClampSampler, uv, 0).r;
        // LinearizeDepth に nearZ, farZ を渡す
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

    // 2. バイナリサーチ
    if (hit)
    {
        float3 minPos = lastRayPos;
        float3 maxPos = rayPos;
        float3 midPos = minPos;

        [unroll(BINARY_SEARCH_STEPS)]
        for (int j = 0; j < BINARY_SEARCH_STEPS; ++j)
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

        // 3. フェード処理
        float2 edgeFade2 = smoothstep(0.0f, 0.05f, hitUV) * smoothstep(1.0f, 0.95f, hitUV);
        float screenEdgeFade = edgeFade2.x * edgeFade2.y;
        float rayDistance = distance(rayOrigin, midPos);
        float rayLengthFade = 1.0f - smoothstep(maxDist * 0.5f, maxDist, rayDistance);

        hitWeight = screenEdgeFade * rayLengthFade * gWaterMaterial.ssrIntensity;

        // 4. サンプリング
        float distFactor = saturate(rayDistance / maxDist);
        float2 distortionOffset = worldNormal.xz * float2(0.04f, -0.04f) * (1.0f - distFactor * 0.5f);
        float2 distortedUV = clamp(hitUV + distortionOffset, 0.005f, 0.995f);
        float mipLevel = roughness * 8.0f + (distFactor * 3.0f);

        return gSceneColorTexture.SampleLevel(gClampSampler, distortedUV, mipLevel).rgb;
    }

    return float3(0.0f, 0.0f, 0.0f);
}

float GenerateProceduralCaustics(float2 uv, float time)
{
    float2x2 m = float2x2(0.866f, -0.5f, 0.5f, 0.866f);
    float intensity = 0.0f;
    float scale = 1.0f;
    float weight = 1.0f;

    [unroll]
    for (int i = 0; i < 3; i++)
    {
        uv = mul(uv, m);
        float2 p = 1.0f - abs(sin(uv * scale + time * 1.5f));
        intensity += (p.x * p.y) * weight;
        scale *= 1.6f;
        weight *= 0.5f;
        time *= 1.2f;
    }
    
    // ★ 5.0f -> 7.0f に変更し、光の線をより細く・鋭くする
    return pow(max(intensity * 0.65f, 0.0f), 7.0f);
}

// ★ 引数に surfaceNormal を追加し、水底の座標(bottomXZ)を受け取るよう変更
float3 CalculateCaustics(float2 bottomXZ, float3 lightDir, float time, float waterDepth, float3 surfaceNormal)
{
    float2 baseUV = bottomXZ * gWaterMaterial.causticsScale + lightDir.xz * (time * 0.03f);
    
    // ★ 水面の法線で水底のUVを歪ませる（水面の波と底の光が完全に連動します）
    baseUV += surfaceNormal.xz * 0.2f;

    // 色ズレ（Chromatic Aberration）を強めに
    float caOffset = 0.05f;
    float r = GenerateProceduralCaustics(baseUV + caOffset, time);
    float g = GenerateProceduralCaustics(baseUV, time);
    float b = GenerateProceduralCaustics(baseUV - caOffset, time);

    // ★ ただの白ではなく、宝石のようなシアン系の色味を足して発光感を出す
    float3 causticsColor = float3(r, g, b) * float3(0.6f, 0.9f, 1.0f);

    float depthFade = exp(-waterDepth / max(gWaterMaterial.causticsFadeDepth, kEpsilon));
    float shoreFade = smoothstep(0.02f, 0.2f, waterDepth);

    // ★ 最後に 2.0f を掛けて「HDR的な強い発光」をさせる
    return causticsColor * gWaterMaterial.causticsIntensity * depthFade * shoreFade * 2.0f;
}

WaterPSOutput main(PixelShaderInput input)
{
    WaterPSOutput output;

    float3 V = normalize(gFrameData.cameraWorldPosition - input.worldPosition);
    
    float clipW = max(input.currentClipPos.w, kEpsilon);
    float2 screenUV = (input.currentClipPos.xy / clipW) * float2(0.5f, -0.5f) + 0.5f;

    float sceneRawDepth = gSceneDepthTexture.Sample(gClampSampler, screenUV).r;
    float sceneLinearDepth = LinearizeDepth(sceneRawDepth, gFrameData.nearClip, gFrameData.farClip);
    float waterLinearDepth = LinearizeDepth(input.currentClipPos.z / clipW, gFrameData.nearClip, gFrameData.farClip);
    float waterDepth = max(sceneLinearDepth - waterLinearDepth, 0.0f);

    float time = gFrameData.gTime * max(gWaterMaterial.waveSpeed, 0.1f);
    
    float2 windDir = gWaterMaterial.windDirection;
    float windLen = length(windDir);
    windDir = (windLen > kEpsilon) ? (windDir / windLen) : float2(0.7071f, 0.7071f);

    float2 uv1 = input.worldPosition.xz * gWaterMaterial.waveTiling.x - windDir * (time * 0.08f);
    float2 windDir2 = float2(windDir.x * 0.866f - windDir.y * 0.5f, windDir.x * 0.5f + windDir.y * 0.866f);
    float2 uv2 = input.worldPosition.xz * gWaterMaterial.waveTiling.y - windDir2 * (time * 0.04f);

    // ★ 法線マップ強度の調整パラメータ(normalIntensity)を適用
    float3 n1 = gWaterNormalMap.Sample(gSampler, uv1).rgb * 2.0f - 1.0f;
    float3 n2 = gWaterNormalMap.Sample(gSampler, uv2).rgb * 2.0f - 1.0f;
    
    n1.xy *= gWaterMaterial.normalIntensity;
    n2.xy *= gWaterMaterial.normalIntensity;

    n1.z = sqrt(saturate(1.0f - dot(n1.xy, n1.xy)));
    n2.z = sqrt(saturate(1.0f - dot(n2.xy, n2.xy)));

    float3 finalTangentNormal = normalize(BlendNormalsRNM(n1, n2));
    
    float3 N = normalize(input.normal);
    float3 T = normalize(input.tangent);
    float3 B = normalize(cross(N, T));
    float3x3 TBN = float3x3(T, B, N);
    float3 worldNormal = normalize(mul(finalTangentNormal, TBN));

    // --- 屈折 ---
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

    // 1. 背景色の吸収と散乱
    float transmittance = exp(-waterDepth * gWaterMaterial.absorption);
    float3 waterBodyColor = lerp(gWaterMaterial.deepColor.rgb, gWaterMaterial.shallowColor.rgb, transmittance);
    
    float scatterFactor = pow(saturate(dot(V, -lightDir)), 4.0f) * (1.0f - transmittance);
    float3 inScattering = gWaterMaterial.scatterColor.rgb * scatterFactor;

    float3 refractedLight = sceneColor * waterBodyColor + inScattering;

    // 2. コースティクス
    float2 bottomXZ = input.worldPosition.xz + (-V.xz) * waterDepth;
    float3 caustics = CalculateCaustics(bottomXZ, lightDir, gFrameData.gTime, waterDepth, worldNormal);
    refractedLight += caustics;

    // --- 反射 (SSR & 環境マップ) ---
    float NdotV = saturate(dot(worldNormal, V));
    float fresnel = 0.02f + (1.0f - 0.02f) * pow(1.0f - NdotV, 5.0f);

    float3 reflectVector = reflect(-V, worldNormal);
    reflectVector.y = max(reflectVector.y, 0.01f);
    reflectVector = normalize(reflectVector);

    float3 skyReflection = gEnvironmentTexture.SampleLevel(gSampler, reflectVector, gWaterMaterial.roughness * 5.0f).rgb;
    skyReflection *= gWaterMaterial.envReflectionIntensity;

    float3 smoothReflectVector = reflect(-V, N);
    smoothReflectVector.y = max(smoothReflectVector.y, 0.01f);
    smoothReflectVector = normalize(smoothReflectVector);

    float ssrWeight = 0.0f;
    float3 ssrColor = TraceSSR_HQ(input.worldPosition, smoothReflectVector, worldNormal, gWaterMaterial.roughness, screenUV, ssrWeight);

    float3 finalReflection = lerp(skyReflection, ssrColor, ssrWeight);
    float edgeFade = smoothstep(0.0f, 0.1f, waterDepth);
    float3 finalColor = lerp(refractedLight, finalReflection, fresnel * edgeFade);

    // ---------------------------------------------------------
    // ★ 泡の完全修正（画面全体が白くなる問題を解決）
    // ---------------------------------------------------------
    // ① 岸辺の泡マスク（浅瀬ほど強くなる）
    float shoreMask = 1.0f - saturate(waterDepth / max(gWaterMaterial.foamThreshold, kEpsilon));
    
    // ② 波頭の泡マスク（波の法線傾斜から判定）
    float waveSlope = 1.0f - finalTangentNormal.z;
    float wavePeakMask = smoothstep(gWaterMaterial.waveFoamThreshold, 1.0f, waveSlope);

    // 泡が発生すべき領域の合成マスク (0.0 ～ 1.0)
    float foamMask = max(shoreMask, wavePeakMask);

    // ③ 泡ノイズの輪郭抽出（一定しきい値以下をカットして透明に保つ）
    float foamNoise = ProceduralFoamNoise(input.worldPosition.xz * gWaterMaterial.foamScale - windDir * (time * 0.1f));
    float foamCutoff = 1.0f - foamMask;
    
    // 境界をクッキリさせて網目状の波紋泡を作る
    float totalFoam = smoothstep(foamCutoff, foamCutoff + 0.15f, foamNoise) * foamMask * gWaterMaterial.foamIntensity;
    totalFoam = saturate(totalFoam);

    finalColor = lerp(finalColor, gWaterMaterial.foamColor.rgb, totalFoam * gWaterMaterial.foamColor.a);

    // --- ハイライト ---
    float3 H = normalize(lightDir + V);
    float spec = pow(saturate(dot(worldNormal, H)), 256.0f / max(gWaterMaterial.roughness, kEpsilon));
    finalColor += gDirectionalLights[0].color.rgb * spec * gWaterMaterial.specularIntensity * edgeFade;

    output.color = float4(finalColor, 1.0f);
    output.normal = float4(worldNormal, 1.0f);
    output.material = float4(gWaterMaterial.roughness, 0.0f, 0.0f, 1.0f);

    float2 currentNDC = input.currentClipPos.xy / input.currentClipPos.w;
    float2 prevNDC = input.prevClipPos.xy / input.prevClipPos.w;
    output.velocity = (currentNDC * float2(0.5f, -0.5f) + 0.5f) - (prevNDC * float2(0.5f, -0.5f) + 0.5f);

    return output;
}