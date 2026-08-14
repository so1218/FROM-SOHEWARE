#include "ShaderConstants.hlsli"
#include "LightingUtils.hlsli"
#include "NormalUtils.hlsli"
#include "PBRUtils.hlsli"
#include "ShadowUtils.hlsli"

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

cbuffer PebbleMaterials : register(b4)
{
    PebbleMaterialData gPebbleMaterials[4]; // kMaxModelTypesと合わせる
};

ConstantBuffer<ShadowData> gShadowData : register(b8);


Texture2D<float4> gAlbedoMap : register(t0);
TextureCube<float4> gEnvironmentTexture : register(t1);
Texture2DArray<float> gShadowMapArray : register(t2);
Texture2D<float4> gNormalMap : register(t3);

SamplerState gSampler : register(s0);
SamplerComparisonState gShadowSampler : register(s1);

struct VertexShaderOutput
{
    float4 position : SV_POSITION; // クリップ空間座標
    float3 worldPosition : POSITION0; // ワールド空間位置
    float2 texcoord : TEXCOORD0; // UV座標
    float3 normal : NORMAL0; // ワールド空間法線
    float3 tangent : TANGENT0; // ワールド空間接線
    float2 velocity : TEXCOORD1; // VSで計算済みのVelocity
    nointerpolation float3 colorVariation : COLOR0;
    float heightFactor : TEXCOORD2;
};

struct PixelShaderOutput
{
    float4 color : SV_Target0;
    float4 normal : SV_Target1;
    float4 material : SV_Target2;
    float2 velocity : SV_Target3;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    PebbleMaterialData matData = gPebbleMaterials[0];

    // --------------------------------------------------------
    // 1. 法線計算 & 地面との法線ブレンド (Normal Blending)
    // --------------------------------------------------------
    float3 normalizedInputNormal = normalize(input.normal);
    
    float3 worldNormal = CalculateNormalFromMap(
        normalizedInputNormal,
        input.tangent,
        input.texcoord,
        matData.normalIntensity,
        gNormalMap,
        gSampler
    );

    // ★ RDR2手法: 地面との接地面（高さが低い部分）の法線を上に向け、ポリゴンの交差感を緩和する
    float wetnessMask = pow(1.0f - input.heightFactor, 2.5f); // 底面近くで急激に強くなるマスク
    worldNormal = normalize(lerp(worldNormal, float3(0.0f, 1.0f, 0.0f), wetnessMask * 0.4f));

    // --------------------------------------------------------
    // 2. Albedo サンプリング & RDR2風 垂直グラデーション
    // --------------------------------------------------------
    float4 albedoSample = gAlbedoMap.Sample(gSampler, input.texcoord);
    float3 rawAlbedo = albedoSample.rgb * matData.baseColor.rgb;

    // ★ RDR2手法: 底面ほど泥（Dark/Earth color）をブレンドして湿り気と境界線を馴染ませる
    float3 mudColor = rawAlbedo * float3(0.25f, 0.2f, 0.15f); // 暗い土色
    float3 baseAlbedo = lerp(rawAlbedo, mudColor, wetnessMask * 0.75f);

    // 個体ごとのカラーバリエーションを適用
    baseAlbedo *= input.colorVariation;

    // --------------------------------------------------------
    // 3. Roughness & Metalness (底面の湿り気表現)
    // --------------------------------------------------------
    // ★ RDR2手法: 底面ほど Roughness を下げて「水・湿気でテカる」質感を演出
    float currentRoughness = clamp(matData.roughness * (1.0f - wetnessMask * 0.5f), 0.05f, 1.0f);
    float currentMetalness = saturate(matData.metalness);

    // --------------------------------------------------------
    // 4. CSM (カスケードシャドウ)
    // --------------------------------------------------------
    float shadowFactor = 1.0f;
    if (gDirectionalLights[0].enable != 0)
    {
        float viewDepth = distance(gFrameData.cameraWorldPosition, input.worldPosition);
        float3 lightDir = normalize(-gDirectionalLights[0].direction);

        shadowFactor = CalculateShadowCSM(
            input.worldPosition, worldNormal, viewDepth, lightDir,
            matData.shadowDensity, gShadowData.cascadeSplits, matData.shadowNormalBias,
            matData.shadowBias, matData.shadowSoftness, gShadowData.cascadeLightViewProj,
            gShadowMapArray, gShadowSampler
        );
    }

    // --------------------------------------------------------
    // 5. SurfaceData 構築 & ライティング
    // --------------------------------------------------------
    SurfaceData surface;
    surface.albedo = baseAlbedo;
    surface.pbrAlbedo = baseAlbedo * pow(abs(baseAlbedo), 2.2f);
    surface.specularColor = float3(1.0f, 1.0f, 1.0f);
    surface.normal = worldNormal;
    surface.roughness = currentRoughness;
    surface.metalness = currentMetalness;
    surface.shininess = matData.shininess;
    surface.diffuseReflection = matData.diffuseReflection;
    surface.lightMode = SHADING_MODEL_PBR;

    float3 toEyeWorld = normalize(gFrameData.cameraWorldPosition - input.worldPosition);
    float3 finalColor = 0.0f.xxx;

    // ① メインライト
    finalColor += ApplyDirectionalLights(surface, toEyeWorld, shadowFactor, gDirectionalLights, gAlbedoMap, gSampler);

    // ② 局所ライト
    finalColor += ApplyPointLights(surface, input.worldPosition, toEyeWorld, gPointLights);
    finalColor += ApplySpotLights(surface, input.worldPosition, toEyeWorld, gSpotLights);

    // ③ IBL (環境光 / スペキュラ反射)
    float3 kS = F_SchlickRoughness(max(dot(surface.normal, toEyeWorld), 0.0f), 0.04f.xxx, surface.roughness);
    float3 kD = (1.0f.xxx - kS) * (1.0f - surface.metalness);
    float ambientOcclusion = lerp(matData.shadowEnvStrength, 1.0f, shadowFactor);
    float3 ambientDiffuse = kD * surface.pbrAlbedo * 0.03f;
    
    float3 reflectionVector = reflect(-toEyeWorld, surface.normal);
    float3 envColor = gEnvironmentTexture.SampleLevel(gSampler, reflectionVector, surface.roughness * 6.0f).rgb;
    float3 F0 = lerp(0.04f.xxx, surface.pbrAlbedo, surface.metalness);
    float3 F_env = F_SchlickRoughness(max(dot(surface.normal, toEyeWorld), 0.0f), F0, surface.roughness);
    float3 ambientSpecular = envColor * F_env;

    float3 ambient = (ambientDiffuse + ambientSpecular) * matData.environmentMapIntensity;
    finalColor += ambient * ambientOcclusion;

    // --------------------------------------------------------
    // 出力
    // --------------------------------------------------------
    output.color = float4(finalColor, 1.0f);
    output.normal = float4(worldNormal, 1.0f);
    output.material = float4(currentMetalness, currentRoughness, 0.0f, 1.0f);
    output.velocity = input.velocity;

    return output;
}