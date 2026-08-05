#include "Object3D.hlsli"
#include "ShaderConstants.hlsli"
#include "ShadowUtils.hlsli"
#include "LightingUtils.hlsli"
#include "NormalUtils.hlsli"
#include "PBRUtils.hlsli"

// 定数バッファやテクスチャのレジスタは Object3D.PS と揃える
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

Texture2D<float4> gTexture : register(t0); // 幹のアルベド
Texture2DArray<float> gShadowMapArray : register(t2);
Texture2D<float4> gToonRamp : register(t3);
Texture2D<float4> gNormalTexture : register(t5); // 幹のノーマルマップ

SamplerState gSampler : register(s0);
SamplerComparisonState gShadowSampler : register(s1);
SamplerState gClampSampler : register(s2);

struct PixelInput
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

// 高速 IGN 
float InterleavedGradientNoise(float2 pixelPos)
{
    float3 magic = float3(0.06711056f, 0.00583715f, 52.9829189f);
    return frac(magic.z * frac(dot(pixelPos, magic.xy)));
}

PixelShaderOutput main(PixelInput input)
{
    PixelShaderOutput output;
    
    // -------------------------------------------------------------------------
    // 1. LODディザリング (IGN化)
    // -------------------------------------------------------------------------
    float dither = InterleavedGradientNoise(input.position.xy);
    clip(input.lodFade - dither);

    // -------------------------------------------------------------------------
    // 2. アルベド & 濡れ (Wetness) 演算
    // -------------------------------------------------------------------------
    float4 textureColor = gTexture.Sample(gSampler, input.texcoord);
    float3 baseColor = textureColor.rgb * input.instanceTint * gMaterial.color.rgb * max(gMaterial.albedoMultiplier, 0.0f);

    float wetness = gEnvironmentData.wetness;
    
    // ① 樹皮の吸光 (Porosity: 濡れると暗く・重みが増す)
    baseColor = lerp(baseColor, baseColor * 0.55f, wetness);

    // ② ラフネス低下 (水膜でのツヤ)
    float baseRoughness = clamp(gMaterial.roughness, 0.05f, 1.0f);
    float roughness = lerp(baseRoughness, 0.2f, wetness);

    // -------------------------------------------------------------------------
    // 3. 法線計算 (Bitangentの動的算出)
    // -------------------------------------------------------------------------
    float3 worldNormal = normalize(input.normal);
    float3 worldTangent = normalize(input.tangent);
    float3 worldBitangent = cross(worldNormal, worldTangent); // PSで動的復元

    float3 normal = worldNormal;
    if (gMaterial.enableNormalMap != 0)
    {
        // ノーマルマップから法線生成 (Bitangentは関数内で動的補正される前提)
        normal = CalculateNormalFromMap(worldNormal, worldTangent, input.texcoord, gMaterial.normalIntensity, gNormalTexture, gSampler);
    }

    float3 toEyeWorld = normalize(gFrameData.cameraWorldPosition - input.worldPosition);

    // -------------------------------------------------------------------------
    // 4. シャドウ計算
    // -------------------------------------------------------------------------
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

    // -------------------------------------------------------------------------
    // 5. SurfaceData構築
    // -------------------------------------------------------------------------
    SurfaceData surface;
    surface.albedo = baseColor;
    surface.pbrAlbedo = baseColor * baseColor; // pow(x, 2.2) の代わりの高速ガンマ近似
    surface.specularColor = gMaterial.specularColor.rgb;
    surface.normal = normal;
    surface.roughness = roughness;
    surface.metalness = saturate(gMaterial.metalness);
    surface.shininess = gMaterial.shininess;
    surface.diffuseReflection = gMaterial.diffuseReflection;
    surface.lightMode = gMaterial.lightMode;

    // -------------------------------------------------------------------------
    // 6. ライティング & 数式環境光 (CubeMapサンプリングゼロ)
    // -------------------------------------------------------------------------
    float3 finalColor = 0.0f.xxx;
    if (gMaterial.enableLighting != 0)
    {
        finalColor += ApplyDirectionalLights(surface, toEyeWorld, shadowFactor, gDirectionalLights, gToonRamp, gClampSampler);
        finalColor += ApplyPointLights(surface, input.worldPosition, toEyeWorld, gPointLights);
        finalColor += ApplySpotLights(surface, input.worldPosition, toEyeWorld, gSpotLights);

        if (gMaterial.lightMode == SHADING_MODEL_PBR)
        {
            float NdotV = max(dot(surface.normal, toEyeWorld), 0.0f);
            
            // F0 (濡れに応じて水のF0=0.02へ遷移)
            float3 baseF0 = lerp(0.04f.xxx, surface.pbrAlbedo, surface.metalness);
            float3 F0 = lerp(baseF0, 0.02f.xxx, wetness);
            
            float3 kS = F_SchlickRoughness(NdotV, F0, surface.roughness);
            float3 kD = (1.0f.xxx - kS) * (1.0f - surface.metalness);

            float combinedAO = lerp(gMaterial.shadowEnvStrength, 1.0f, shadowFactor) * input.color.a;

            // ★ CubeMapサンプリングの代わりに「天空光/地面光の半球ライティング」を使用
            float3 reflectDir = reflect(-toEyeWorld, surface.normal);
            float skyWeight = saturate(reflectDir.y * 0.5f + 0.5f);
            float3 envSkyColor = lerp(gEnvironmentData.groundColor.rgb, gEnvironmentData.skyColor.rgb, skyWeight);

            // 環境光Specular
            float3 ambientSpecular = envSkyColor * kS;

            // 環境光Diffuse
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

    // -------------------------------------------------------------------------
    // 7. 出力
    // -------------------------------------------------------------------------
    output.color = float4(finalColor, 1.0f);
    output.normal = float4(normal, 1.0f);
    output.material = float4(surface.metalness, surface.roughness, 0.0f, 1.0f);
    output.velocity = float2(0.0f, 0.0f);
    
    return output;
}