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
Texture2D<float4> gRippleTexture : register(t4);
Texture2D<float4> gInteractionMap : register(t11);

SamplerState gSampler : register(s0);
SamplerState gClampSampler : register(s1);

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

float2 VoronoiHash(float2 p)
{
    p = float2(dot(p, float2(127.1f, 311.7f)), dot(p, float2(269.5f, 183.3f)));
    return frac(sin(p) * 43758.5453123f);
}

// ---------------------------------------------------------
// ★ AAA級 網目状ボロノイエッジ関数 (輝度完全安定版)
// ---------------------------------------------------------
float CellularCausticsEdge(float2 uv)
{
    float2 g = floor(uv);
    float2 f = frac(uv);
    
    float minDist1 = 1.0f; // 最も近い距離 (F1)
    float minDist2 = 1.0f; // 2番目に近い距離 (F2)

    [unroll]
    for (int y = -1; y <= 1; y++)
    {
        [unroll]
        for (int x = -1; x <= 1; x++)
        {
            float2 lattice = float2(x, y);
            // ★ 時間によるsin動的移動を廃止（ボロノイ核を固定化）
            // これにより画面全体の平均輝度が数学的に完全一定になります
            float2 offset = VoronoiHash(g + lattice);
            float2 distVec = lattice + offset - f;
            
            float d = dot(distVec, distVec);
            
            // F1とF2のソート更新
            if (d < minDist1)
            {
                minDist2 = minDist1;
                minDist1 = d;
            }
            else if (d < minDist2)
            {
                minDist2 = d;
            }
        }
    }
    
    // ★ 境界線（エッジ）の抽出
    float edgeDist = sqrt(minDist2) - sqrt(minDist1);
    
    return pow(1.0f - smoothstep(0.0f, 0.15f, edgeDist), 4.0f);
}

// ---------------------------------------------------------
// ★ AAA級 物理ベースコースティクス計算関数（流体UV移動版）
// ---------------------------------------------------------
float3 CalculateCausticsAAA(float3 bottomWorldPos, float3 worldNormal, float3 lightDir, float time, float waterDepth)
{
    float eta = 1.0f / 1.333f;
    float3 refractedLightDir = refract(-lightDir, worldNormal, eta);
    if (length(refractedLightDir) < 0.001f)
    {
        refractedLightDir = -lightDir;
    }

    // 基本スケールと速度
    float scale = gWaterMaterial.causticsScale * 0.15f;
    float speed = time * gWaterMaterial.causticsSpeed * 1.2f;
    float2 baseXZ = bottomWorldPos.xz * scale;

    // 1. gRippleTexture によるドメインワーピング（流体のうねり）
    float2 warpUV1 = baseXZ * 0.5f + float2(speed * 0.03f, speed * 0.02f);
    float2 warpUV2 = baseXZ * 0.8f + float2(-speed * 0.02f, speed * 0.04f);
    
    float2 warp1 = gRippleTexture.Sample(gSampler, warpUV1).rg * 2.0f - 1.0f;
    float2 warp2 = gRippleTexture.Sample(gSampler, warpUV2).rg * 2.0f - 1.0f;
    float2 totalWarp = (warp1 + warp2 * 0.5f) * gWaterMaterial.causticsDistortion * 0.3f;

    // 2. 歪ませたUVに進行方向のスクロールを加算
    float2 uv = baseXZ + totalWarp + float2(speed * 0.05f, speed * 0.03f);

    // 3. 自然な色収差の計算（静的ボロノイの呼び出し）
    float dispersion = 0.015f * gWaterMaterial.causticsDistortion;
    
    float r = CellularCausticsEdge(uv + totalWarp * dispersion);
    float g = CellularCausticsEdge(uv);
    float b = CellularCausticsEdge(uv - totalWarp * dispersion);
    
    float3 colorFringe = float3(r, g, b); // フチの虹色
    float3 whiteCore = float3(g, g, g); // 中心部の強い白光

    // 4. メインの色合成
    float3 finalCausticsColor = lerp(whiteCore, colorFringe, 0.4f);

    // 太陽光の入射角による減衰
    float lightFactor = saturate(dot(float3(0, 1, 0), lightDir));

    // 太陽光の色（gDirectionalLights[0].color.rgb）を乗算
    float3 sunColor = gDirectionalLights[0].color.rgb;
    
    return finalCausticsColor * sunColor * gWaterMaterial.causticsIntensity * lightFactor * 5.0f;
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

    // ---------------------------------------------------------
    // 基礎風波法線
    // ---------------------------------------------------------
    float2 uv1 = input.worldPosition.xz * gWaterMaterial.waveTiling.x - windDir * (time * 0.08f);
    float2 windDir2 = float2(windDir.x * 0.866f - windDir.y * 0.5f, windDir.x * 0.5f + windDir.y * 0.866f);
    float2 uv2 = input.worldPosition.xz * gWaterMaterial.waveTiling.y - windDir2 * (time * 0.04f);

    float3 n1 = gWaterNormalMap.Sample(gSampler, uv1).rgb * 2.0f - 1.0f;
    float3 n2 = gWaterNormalMap.Sample(gSampler, uv2).rgb * 2.0f - 1.0f;
    
    n1.xy *= gWaterMaterial.normalIntensity;
    n2.xy *= gWaterMaterial.normalIntensity;
    n1.z = sqrt(saturate(1.0f - dot(n1.xy, n1.xy)));
    n2.z = sqrt(saturate(1.0f - dot(n2.xy, n2.xy)));

    float3 finalTangentNormal = normalize(BlendNormalsRNM(n1, n2));

    // ---------------------------------------------------------
    // 高精細インタラクション波紋法線
    // ---------------------------------------------------------
    float2 interactUV = (input.worldPosition.xz - gInteractionData.centerWorldPos) / gInteractionData.worldSize + 0.5f;
    float4 interactData = float4(0.5f, 0.5f, 0.0f, 0.0f);

    if (all(interactUV >= 0.0f) && all(interactUV <= 1.0f))
    {
        interactData = gInteractionMap.Sample(gClampSampler, interactUV);
    }

    float2 pushDirXZ = (interactData.rg * 2.0f) - 1.0f;
    float interactTrail = interactData.a;

    float3 interactNormal = float3(pushDirXZ * interactTrail * gWaterMaterial.interactionNormalScale, 1.0f);
    interactNormal = normalize(interactNormal);

    finalTangentNormal = normalize(BlendNormalsRNM(finalTangentNormal, interactNormal));
    
    float3 N = normalize(input.normal);
    float3 T = normalize(input.tangent);
    float3 B = normalize(cross(N, T));
    float3x3 TBN = float3x3(T, B, N);
    float3 worldNormal = normalize(mul(finalTangentNormal, TBN));

    worldNormal.y = max(worldNormal.y, 0.15f);
    worldNormal = normalize(worldNormal);

    // ---------------------------------------------------------
    // 屈折とカラー
    // ---------------------------------------------------------
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

    // ★ 1. 相似比を利用したカメラ完全独立のTerrain 3Dワールド座標復元
    // カメラから水面頂点へのベクトルに「水底深度 / 水面深度」の比率を掛けることで正確な水底位置を特定
    float depthRatio = sceneLinearDepth / max(waterLinearDepth, kEpsilon);
    float3 bottomWorldPos = gFrameData.cameraWorldPosition + (input.worldPosition - gFrameData.cameraWorldPosition) * depthRatio;

    // ★ 2. 復元した固定ワールド座標に対してコースティクスを照射
    float3 caustics = CalculateCausticsAAA(bottomWorldPos, worldNormal, lightDir, gFrameData.gTime, waterDepth);

    // ★ 3. 地形表面の光線合成と水中光吸収 (Beer-Lambert) の適用
    float3 illuminatedTerrain = sceneColor + (sceneColor * caustics) + (caustics * 0.2f);

    float transmittance = exp(-waterDepth * gWaterMaterial.absorption);
    float3 waterBodyColor = lerp(gWaterMaterial.deepColor.rgb, gWaterMaterial.shallowColor.rgb, transmittance);
    
    float waveSubsurface = saturate(dot(V, -lightDir) + 0.5f) * (interactTrail * 0.8f + (1.0f - finalTangentNormal.z) * 0.5f);
    float scatterFactor = pow(saturate(dot(V, -lightDir)), 4.0f) * (1.0f - transmittance);
    float3 inScattering = gWaterMaterial.scatterColor.rgb * (scatterFactor + waveSubsurface * 1.5f);

    float3 refractedLight = illuminatedTerrain * waterBodyColor + inScattering;

    // ---------------------------------------------------------
    // 反射 (SSR & 環境マップ)
    // ---------------------------------------------------------
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
    float3 ssrColor = TraceSSR_HQ(input.worldPosition, smoothReflectVector, worldNormal, gWaterMaterial.roughness, screenUV, ssrWeight);

    float3 finalReflection = lerp(skyReflection, ssrColor, ssrWeight);
    float edgeFade = smoothstep(0.0f, 0.1f, waterDepth);
    float3 finalColor = lerp(refractedLight, finalReflection, fresnel * edgeFade);

    // ---------------------------------------------------------
    // 泡の合成
    // ---------------------------------------------------------
    float shoreMask = 1.0f - saturate(waterDepth / max(gWaterMaterial.foamThreshold, kEpsilon));
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

    // スペキュラハイライト
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