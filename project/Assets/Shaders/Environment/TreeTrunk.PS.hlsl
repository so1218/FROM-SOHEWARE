#include "Common/Object3D.hlsli"
#include "Common/ShaderConstants.hlsli"
#include "Common/ShadowUtils.hlsli"
#include "Common/LightingUtils.hlsli"
#include "Common/NormalUtils.hlsli"
#include "Common/PBRUtils.hlsli"

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

ConstantBuffer<GlobalEnvironmentData> gEnvironmentData : register(b4);
ConstantBuffer<TrunkMaterialData> gMaterial : register(b6);
ConstantBuffer<ShadowData> gShadowData : register(b8);

Texture2D<float4> gTexture : register(t0);
Texture2DArray<float> gShadowMapArray : register(t2);
Texture2D<float4> gToonRamp : register(t3);
Texture2D<float4> gNormalTexture : register(t5);

SamplerState gSampler : register(s0);
SamplerComparisonState gShadowSampler : register(s1);
SamplerState gClampSampler : register(s2);

struct TreeTrunkPSInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 worldPosition : WORLD_POSITION;
    float4 color : COLOR0;
    float3 instanceTint : COLOR1;
    float lodFade : BLENDWEIGHT;
};

PixelShaderOutput main(TreeTrunkPSInput input)
{
    PixelShaderOutput output;

    // カメラからの距離計算
    float3 cameraDiff = gFrameData.cameraWorldPosition - input.worldPosition;
    float viewDepth = length(cameraDiff);

    // カメラ近接フェード率の算出
    float proximityFade = saturate((viewDepth - gMaterial.nearFadeMinDist) / max(gMaterial.nearFadeMaxDist - gMaterial.nearFadeMinDist, kEpsilon));

    // LODクロスフェードと近接フェードの合成
    float finalFade = min(input.lodFade, proximityFade);

    // ディザリングによるLOD/近接クロスフェード
    float dither = InterleavedGradientNoise(input.position.xy);
    clip(finalFade - dither);

    // テクスチャサンプリングおよびベースカラーの構築
    float4 textureColor = gTexture.Sample(gSampler, input.texcoord);
    float3 baseColor = textureColor.rgb * input.instanceTint * gMaterial.color.rgb * max(gMaterial.albedoMultiplier, 0.0f);

    float wetness = gEnvironmentData.wetness;

    // 雨天時の樹皮変化 (内部散乱による暗転と表面水膜によるラフネス低下を反映)
    baseColor = lerp(baseColor, baseColor * 0.55f, wetness);
    float baseRoughness = clamp(gMaterial.roughness, 0.05f, 1.0f);
    float roughness = lerp(baseRoughness, 0.2f, wetness);

    // 法線計算
    float3 worldNormal = normalize(input.normal);
    float3 worldTangent = normalize(input.tangent);

    float3 normal = worldNormal;
    if (gMaterial.enableNormalMap != 0)
    {
        normal = CalculateNormalFromMap(worldNormal, worldTangent, input.texcoord, gMaterial.normalIntensity, gNormalTexture, gSampler);
    }

    float3 toEyeWorld = normalize(gFrameData.cameraWorldPosition - input.worldPosition);

    // CSMによる影判定
    float shadowFactor = 1.0f;
    if (gDirectionalLights[0].enable && gMaterial.addShadow != 0)
    {
        float viewDepth = distance(gFrameData.cameraWorldPosition, input.worldPosition);
        float3 lightDir = normalize(-gDirectionalLights[0].direction);
        shadowFactor = CalculateShadowCSM(
            input.worldPosition, normal, viewDepth, lightDir,
            gMaterial.shadowDensity, gShadowData.cascadeSplits,
            gMaterial.shadowNormalBias, gMaterial.shadowBias, gMaterial.shadowSoftness,
            gShadowData.cascadeLightViewProj, gShadowMapArray, gShadowSampler);
    }

    // PBRサーフェスデータの再構築
    SurfaceData surface;
    surface.albedo = baseColor;
    surface.pbrAlbedo = baseColor * baseColor; // sRGB to Linear
    surface.specularColor = gMaterial.specularColor.rgb;
    surface.normal = normal;
    surface.roughness = roughness;
    surface.metalness = saturate(gMaterial.metalness);
    surface.shininess = gMaterial.shininess;
    surface.diffuseReflection = gMaterial.diffuseReflection;
    surface.lightMode = gMaterial.lightMode;

    // ライティングおよび簡易IBLの統合
    float3 finalColor = float3(0.0f, 0.0f, 0.0f);
    if (gMaterial.enableLighting != 0)
    {
        finalColor += ApplyDirectionalLights(surface, toEyeWorld, shadowFactor, gDirectionalLights, gToonRamp, gClampSampler);
        finalColor += ApplyPointLights(surface, input.worldPosition, toEyeWorld, gPointLights);
        finalColor += ApplySpotLights(surface, input.worldPosition, toEyeWorld, gSpotLights);

        if (gMaterial.lightMode == SHADING_MODEL_PBR)
        {
            float NdotV = max(dot(surface.normal, toEyeWorld), 0.0f);

            // 水分付着時の屈折率変動(を考慮し、F0を水のフレネル反射率へと推移
            float3 baseF0 = lerp(float3(0.04f, 0.04f, 0.04f), surface.pbrAlbedo, surface.metalness);
            float3 F0 = lerp(baseF0, float3(0.02f, 0.02f, 0.02f), wetness);

            float3 kS = F_SchlickRoughness(NdotV, F0, surface.roughness);
            float3 kD = (float3(1.0f, 1.0f, 1.0f) - kS) * (1.0f - surface.metalness);

            float combinedAO = lerp(gMaterial.shadowEnvStrength, 1.0f, shadowFactor) * input.color.a;

            // 天空光と地表光のグラデーションによる半球アンビエント近似
            float3 reflectDir = reflect(-toEyeWorld, surface.normal);
            float skyWeight = saturate(reflectDir.y * 0.5f + 0.5f);
            float3 envSkyColor = lerp(gEnvironmentData.groundColor.rgb, gEnvironmentData.skyColor.rgb, skyWeight);

            float3 ambientSpecular = envSkyColor * kS;

            float skyLight = saturate(surface.normal.y * 0.5f + 0.5f);
            float3 ambientDiffuse = kD * surface.pbrAlbedo * lerp(gEnvironmentData.groundColor.rgb, gEnvironmentData.skyColor.rgb, skyLight);

            float3 ambient = (ambientDiffuse + ambientSpecular) * gMaterial.environmentMapIntensity;

            finalColor += ambient * combinedAO;
        }
    }
    else
    {
        finalColor = surface.albedo;
    }

    // MRT 出力
    output.color = float4(finalColor, 1.0f);
    output.normal = float4(normal, 1.0f);
    output.material = float4(surface.metalness, surface.roughness, 0.0f, 1.0f);

    return output;
}