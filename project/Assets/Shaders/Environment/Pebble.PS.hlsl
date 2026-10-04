#include "Common/ShaderConstants.hlsli"
#include "Common/LightingUtils.hlsli"
#include "Common/NormalUtils.hlsli"
#include "Common/PBRUtils.hlsli"
#include "Common/ShadowUtils.hlsli"

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
    PebbleMaterialData gPebbleMaterials[4];
};

ConstantBuffer<ShadowData> gShadowData : register(b8);

Texture2D<float4> gAlbedoMap : register(t0);
TextureCube<float4> gEnvironmentTexture : register(t1);
Texture2DArray<float> gShadowMapArray : register(t2);
Texture2D<float4> gNormalMap : register(t3);

SamplerState gSampler : register(s0);
SamplerComparisonState gShadowSampler : register(s1);

struct PebbleVSOutput
{
    float4 position : SV_POSITION;
    float3 worldPosition : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
    float3 tangent : TANGENT0;
    nointerpolation float3 colorVariation : COLOR0;
    float heightFactor : TEXCOORD2;
};

struct PebblePSOutput
{
    float4 color : SV_Target0;
    float4 normal : SV_Target1;
    float4 material : SV_Target2;
};

PebblePSOutput main(PebbleVSOutput input)
{
    PebblePSOutput output;

    // TODO: 現在はインデックス0固定だが、CS/VSからマテリアルIDを渡し、
    // 苔むした石や乾いた石など、複数種類のプロパティを出し分けられるようにする
    PebbleMaterialData matData = gPebbleMaterials[0];

    // --------------------------------------------------------
    // ジオメトリ結合部の擬似ブレンド
    // --------------------------------------------------------
    // NOTE: VSで計算した heightFactor (0=底面, 1=天頂) を使い、
    // 地形との交差部分の法線・カラー・ラフネスを変化させてポリゴンの刺さっている感を緩和
    float wetnessMask = pow(1.0f - input.heightFactor, 2.5f);

    float3 normalizedInputNormal = normalize(input.normal);
    float3 worldNormal = CalculateNormalFromMap(
        normalizedInputNormal, input.tangent, input.texcoord,
        matData.normalIntensity, gNormalMap, gSampler
    );

    // 底面の法線を上（地形法線方向）へ向けることで、ライティングの境界線の違和感を潰す
    worldNormal = normalize(lerp(worldNormal, float3(0.0f, 1.0f, 0.0f), wetnessMask * 0.4f));

    float4 albedoSample = gAlbedoMap.Sample(gSampler, input.texcoord);
    float3 rawAlbedo = albedoSample.rgb * matData.baseColor.rgb;

    // 接地部ほど暗い土色を混ぜ、地面からの泥汚れと湿気を表現
    // TODO: mudColorは固定値ではなく、直下の地形のアルベドマップをサンプリングして馴染ませるのが理想
    float3 mudColor = rawAlbedo * float3(0.25f, 0.2f, 0.15f);
    float3 baseAlbedo = lerp(rawAlbedo, mudColor, wetnessMask * 0.75f);
    baseAlbedo *= input.colorVariation;

    // 水分による光沢（ラフネス低下）の近似
    float currentRoughness = clamp(matData.roughness * (1.0f - wetnessMask * 0.5f), 0.05f, 1.0f);
    float currentMetalness = saturate(matData.metalness);

    // --------------------------------------------------------
    // ライティング & シャドウ
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

    SurfaceData surface;
    surface.albedo = baseAlbedo;
    surface.pbrAlbedo = baseAlbedo * pow(abs(baseAlbedo), 2.2f); // sRGB -> Linear
    surface.specularColor = float3(1.0f, 1.0f, 1.0f);
    surface.normal = worldNormal;
    surface.roughness = currentRoughness;
    surface.metalness = currentMetalness;
    surface.shininess = matData.shininess;
    surface.diffuseReflection = matData.diffuseReflection;
    surface.lightMode = SHADING_MODEL_PBR;

    float3 toEyeWorld = normalize(gFrameData.cameraWorldPosition - input.worldPosition);
    float3 finalColor = 0.0f.xxx;

    finalColor += ApplyDirectionalLights(surface, toEyeWorld, shadowFactor, gDirectionalLights, gAlbedoMap, gSampler);
    finalColor += ApplyPointLights(surface, input.worldPosition, toEyeWorld, gPointLights);
    finalColor += ApplySpotLights(surface, input.worldPosition, toEyeWorld, gSpotLights);

    // NOTE: IBL
    // カスケードシャドウで影になっている領域は、環境光も減衰させて不自然な発光を防ぐ
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

    // TAA/モーションブラー用のVelocityや、後段のSSR等のためのマテリアル情報をMRT出力
    output.color = float4(finalColor, 1.0f);
    output.normal = float4(worldNormal, 1.0f);
    output.material = float4(currentMetalness, currentRoughness, 0.0f, 1.0f);

    return output;
}