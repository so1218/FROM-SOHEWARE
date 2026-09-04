#include "Object3D.hlsli"
#include "ShaderConstants.hlsli"
#include "LightingUtils.hlsli"
#include "PBRUtils.hlsli"

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

float LinearizeDepth(float depth)
{
    float nearP = gFrameData.nearClip;
    float farP = gFrameData.farClip;
    return (nearP * farP) / max(farP - depth * (farP - nearP), 0.00001f);
}

float3 BlendNormalsRNM(float3 baseNormal, float3 detailNormal)
{
    float3 t = baseNormal + float3(0.0f, 0.0f, 1.0f);
    float3 u = detailNormal * float3(-1.0f, -1.0f, 1.0f);
    return t * dot(t, u) / max(t.z, 0.0001f) - u;
}

float Hash21(float2 p)
{
    p = frac(p * float2(123.34f, 456.21f));
    p += dot(p, p + 45.32f);
    return frac(p.x * p.y);
}

float ProceduralFoamNoise(float2 uv)
{
    float2 i = floor(uv);
    float2 f = frac(uv);
    f = f * f * (3.0f - 2.0f * f);

    float a = Hash21(i);
    float b = Hash21(i + float2(1.0f, 0.0f));
    float c = Hash21(i + float2(0.0f, 1.0f));
    float d = Hash21(i + float2(1.0f, 1.0f));

    return lerp(lerp(a, b, f.x), lerp(c, d, f.x), f.y);
}

// ★ スクリーン空間レイマーチングによる SSR (Screen Space Reflection) 計算
float3 TraceSSR(float3 rayOrigin, float3 rayDir, out float hitWeight)
{
    hitWeight = 0.0f;
    
    static const int MAX_STEPS = 24; // レイマーチのステップ数 (精度と負荷のバランス)
    static const float STEP_SIZE = 0.3f; // 1ステップあたりの進む距離 (メートル)

    float3 currentRayPos = rayOrigin;

    [unroll(24)]
    for (int i = 0; i < MAX_STEPS; ++i)
    {
        currentRayPos += rayDir * STEP_SIZE;

        // ワールド座標 -> クリップ空間 -> スクリーンUV変換
        float4 projPos = mul(float4(currentRayPos, 1.0f), gFrameData.viewProjectionMatrix);
        if (projPos.w <= 0.001f)
            break;

        float3 ndc = projPos.xyz / projPos.w;
        float2 uv = ndc.xy * float2(0.5f, -0.5f) + 0.5f;

        // 画面外チェック
        if (uv.x < 0.001f || uv.x > 0.999f || uv.y < 0.001f || uv.y > 0.999f)
        {
            break;
        }

        // 深度テスト
        float sceneRawDepth = gSceneDepthTexture.SampleLevel(gClampSampler, uv, 0).r;
        float sceneLinearDepth = LinearizeDepth(sceneRawDepth);
        float rayLinearDepth = LinearizeDepth(ndc.z);

        float depthDiff = rayLinearDepth - sceneLinearDepth;

        // レイがオブジェクト背後に進入し、かつ交差幅（厚み）以内にあるか判定
        if (depthDiff > 0.01f && depthDiff < gWaterMaterial.ssrThickness)
        {
            // 画面端フェード（画面縁での急激な途切れを防ぎ Cubemap へ滑らかに遷移）
            float2 edgeFade = smoothstep(0.0f, 0.15f, uv) * smoothstep(1.0f, 0.85f, uv);
            float borderWeight = edgeFade.x * edgeFade.y;

            // 距離フェード（遠くのレイほど減衰）
            float distWeight = 1.0f - saturate((float) i / (float) MAX_STEPS);

            hitWeight = borderWeight * distWeight * gWaterMaterial.ssrIntensity;
            return gSceneColorTexture.SampleLevel(gClampSampler, uv, 0).rgb;
        }
    }

    return float3(0.0f, 0.0f, 0.0f);
}

float3 CalculateCaustics(float2 worldXZ, float3 lightDir, float time, float waterDepth)
{
    float2 uv1 = worldXZ * gWaterMaterial.causticsScale + lightDir.xz * (time * 0.05f);
    float2 uv2 = worldXZ * (gWaterMaterial.causticsScale * 1.3f) - lightDir.xz * (time * 0.07f);

    float3 n1 = gWaterNormalMap.Sample(gSampler, uv1).rgb * 2.0f - 1.0f;
    float3 n2 = gWaterNormalMap.Sample(gSampler, uv2).rgb * 2.0f - 1.0f;

    float3 combinedN = normalize(n1 + n2);
    float causticsPattern = pow(saturate(dot(combinedN, float3(0.0f, 1.0f, 0.0f))), 8.0f);

    float depthFade = exp(-waterDepth / max(gWaterMaterial.causticsFadeDepth, 0.01f)) * smoothstep(0.02f, 0.2f, waterDepth);
    return causticsPattern * gWaterMaterial.causticsIntensity * depthFade * gDirectionalLights[0].color.rgb;
}

WaterPSOutput main(PixelShaderInput input)
{
    WaterPSOutput output;

    float3 V = normalize(gFrameData.cameraWorldPosition - input.worldPosition);
    float2 screenUV = (input.currentClipPos.xy / input.currentClipPos.w) * float2(0.5f, -0.5f) + 0.5f;

    // 1. 深度計算
    float sceneRawDepth = gSceneDepthTexture.Sample(gSampler, screenUV).r;
    float sceneLinearDepth = LinearizeDepth(sceneRawDepth);
    float waterLinearDepth = LinearizeDepth(input.currentClipPos.z / input.currentClipPos.w);
    float waterDepth = max(sceneLinearDepth - waterLinearDepth, 0.0f);

    // 2. 法線計算
    float time = gFrameData.gTime * gWaterMaterial.waveSpeed;
    float2 uv1 = input.worldPosition.xz * gWaterMaterial.waveTiling.x + float2(0.01f, 0.015f) * time;
    float2 uv2 = input.worldPosition.xz * gWaterMaterial.waveTiling.y + float2(-0.02f, 0.01f) * time;

    float3 n1 = gWaterNormalMap.Sample(gSampler, uv1).rgb * 2.0f - 1.0f;
    float3 n2 = gWaterNormalMap.Sample(gSampler, uv2).rgb * 2.0f - 1.0f;
    n1.z = sqrt(saturate(1.0f - dot(n1.xy, n1.xy)));
    n2.z = sqrt(saturate(1.0f - dot(n2.xy, n2.xy)));

    float3 finalTangentNormal = normalize(BlendNormalsRNM(n1, n2));

    float3 N = normalize(input.normal);
    float3 T = normalize(input.tangent);
    float3 B = normalize(cross(N, T));
    float3x3 TBN = float3x3(T, B, N);
    float3 worldNormal = normalize(mul(finalTangentNormal, TBN));

    // 3. 屈折 (Refraction)
    float distortion = smoothstep(0.0f, 0.5f, waterDepth) * gWaterMaterial.refractionAmount;
    float caOffset = gWaterMaterial.chromaticAberration * 0.01f;

    float2 refractUV_R = screenUV + finalTangentNormal.xy * (distortion + caOffset);
    float2 refractUV_G = screenUV + finalTangentNormal.xy * distortion;
    float2 refractUV_B = screenUV + finalTangentNormal.xy * (distortion - caOffset);

    if (LinearizeDepth(gSceneDepthTexture.Sample(gSampler, refractUV_G).r) < waterLinearDepth)
    {
        refractUV_R = refractUV_G = refractUV_B = screenUV;
    }

    float3 sceneColor;
    sceneColor.r = gSceneColorTexture.Sample(gSampler, refractUV_R).r;
    sceneColor.g = gSceneColorTexture.Sample(gSampler, refractUV_G).g;
    sceneColor.b = gSceneColorTexture.Sample(gSampler, refractUV_B).b;

    // 4. 動的コースティクス
    float3 lightDir = normalize(-gDirectionalLights[0].direction);
    float3 caustics = CalculateCaustics(input.worldPosition.xz, lightDir, gFrameData.gTime, waterDepth);
    sceneColor += caustics;

    // 5. Beer-Lambert & 水中散乱
    float transmittance = exp(-waterDepth * gWaterMaterial.absorption);
    float3 waterBodyColor = lerp(gWaterMaterial.deepColor.rgb, gWaterMaterial.shallowColor.rgb, transmittance);
    
    float scatterFactor = pow(saturate(dot(V, -lightDir)), 4.0f) * (1.0f - transmittance);
    float3 inScattering = gWaterMaterial.scatterColor.rgb * scatterFactor;

    float3 refractedLight = sceneColor * waterBodyColor + inScattering;

    // 6. 反射合成 (SSR + キューブマップ環境反射)
    float NdotV = saturate(dot(worldNormal, V));
    float fresnel = 0.02f + (1.0f - 0.02f) * pow(1.0f - NdotV, 5.0f);

    float3 reflectVector = reflect(-V, worldNormal);

    // [A] キューブマップ反射 (バックアップ・背景用)
    float3 skyReflection = gEnvironmentTexture.SampleLevel(gSampler, reflectVector, gWaterMaterial.roughness * 5.0f).rgb;
    skyReflection *= gWaterMaterial.envReflectionIntensity;

    // [B] SSR (動的スクリーン空間反射)
    float ssrWeight = 0.0f;
    float3 ssrColor = TraceSSR(input.worldPosition, reflectVector, ssrWeight);

    // SSRのヒット状況に応じてキューブマップと滑らかにブレンド
    float3 finalReflection = lerp(skyReflection, ssrColor, ssrWeight);

    float edgeFade = smoothstep(0.0f, 0.1f, waterDepth);
    float3 finalColor = lerp(refractedLight, finalReflection, fresnel * edgeFade);

    // 7. 泡 (Foam)
    float foamNoise = ProceduralFoamNoise(input.worldPosition.xz * gWaterMaterial.foamScale + float2(time * 0.1f, time * 0.05f));
    float shoreFoam = smoothstep(gWaterMaterial.foamThreshold, 0.0f, waterDepth);
    float wavePeakFoam = smoothstep(0.6f, 1.0f, input.normal.y) * saturate(worldNormal.y - 0.8f);
    
    float totalFoam = saturate((shoreFoam + wavePeakFoam * 0.5f) * foamNoise * gWaterMaterial.foamIntensity);
    finalColor = lerp(finalColor, gWaterMaterial.foamColor.rgb, totalFoam * gWaterMaterial.foamColor.a);

    // 8. ハイライト
    float3 H = normalize(lightDir + V);
    float spec = pow(saturate(dot(worldNormal, H)), 256.0f / max(gWaterMaterial.roughness, 0.001f));
    finalColor += gDirectionalLights[0].color.rgb * spec * gWaterMaterial.specularIntensity * edgeFade;

    output.color = float4(finalColor, 1.0f);
    output.normal = float4(worldNormal, 1.0f);
    output.material = float4(gWaterMaterial.roughness, 0.0f, 0.0f, 1.0f);

    float2 currentNDC = input.currentClipPos.xy / input.currentClipPos.w;
    float2 prevNDC = input.prevClipPos.xy / input.prevClipPos.w;
    output.velocity = (currentNDC * float2(0.5f, -0.5f) + 0.5f) - (prevNDC * float2(0.5f, -0.5f) + 0.5f);

    return output;
}