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
ConstantBuffer<MaterialData> gMaterial : register(b5); // 幹用のマテリアルデータ
ConstantBuffer<ShadowData> gShadowData : register(b8);

Texture2D<float4> gTexture : register(t0); // 幹のアルベド
TextureCube<float4> gEnvironmentTexture : register(t1);
Texture2DArray<float> gShadowMapArray : register(t2);
Texture2D<float4> gNormalTexture : register(t5); // 幹のノーマルマップ

SamplerState gSampler : register(s0);
SamplerComparisonState gShadowSampler : register(s1);

struct PixelInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
    float3 worldPosition : WORLD_POSITION;
    float4 color : COLOR0; // GustMask と 擬似AO をPSに渡すために使用
    float3 instanceTint : COLOR1;
    float lodFade : BLENDWEIGHT;
};

PixelShaderOutput main(PixelInput input)
{
    PixelShaderOutput output;
    
    // 1. 木特有の処理（LODクロスフェード用ディザリング）
    float dither = frac(sin(dot(input.position.xy, float2(12.9898f, 78.233f))) * 43758.5453f);
    clip(input.lodFade - dither);

    // 2. テクスチャサンプリング (POMやTriplanarは幹には重すぎる/不要なので通常のUVを使う)
    float4 textureColor = gTexture.Sample(gSampler, input.texcoord);
    
    // 幹は不透明なのでアルファテスト(discard)は削除
    
    // 木ごとの色ブレ(Instance Tint)を適用
    float3 baseColor = textureColor.rgb * input.instanceTint;

    float3 worldNormal = normalize(input.normal);
    float3 toEyeWorld = normalize(gFrameData.cameraWorldPosition - input.worldPosition);

    // 3. Normal Map
    float3 normal = worldNormal;
    if (gMaterial.enableNormalMap != 0)
    {
        // CalculateNormalFromMap関数のPOM用UVには通常のUVを渡す
        normal = CalculateNormalFromMap(worldNormal, input.tangent, input.texcoord, gMaterial.normalIntensity, gNormalTexture, gSampler);
    }

    // 4. Shadow
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

    // 5. SurfaceData構築
    SurfaceData surface;
    surface.albedo = baseColor * gMaterial.color.rgb;
    surface.pbrAlbedo = baseColor * pow(abs(gMaterial.color.rgb), 2.2f);
    surface.specularColor = gMaterial.specularColor.rgb;
    surface.normal = normal;
    surface.roughness = clamp(gMaterial.roughness, 0.05f, 1.0f);
    surface.metalness = saturate(gMaterial.metalness);
    surface.shininess = gMaterial.shininess;
    surface.diffuseReflection = gMaterial.diffuseReflection;
    surface.lightMode = gMaterial.lightMode;

    // 6. Lighting & PBR (Object3D.PSから移植)
    float3 finalColor = 0.0f.xxx;
    if (gMaterial.enableLighting != 0)
    {
        // 各種ライト適用
        finalColor += ApplyDirectionalLights(surface, toEyeWorld, shadowFactor, gDirectionalLights, gToonRamp, gClampSampler);
        finalColor += ApplyPointLights(surface, input.worldPosition, toEyeWorld, gPointLights);
        finalColor += ApplySpotLights(surface, input.worldPosition, toEyeWorld, gSpotLights);

        if (gMaterial.lightMode == SHADING_MODEL_PBR)
        {
            float3 kS = F_SchlickRoughness(max(dot(surface.normal, toEyeWorld), 0.0f), 0.04f.xxx, surface.roughness);
            float3 kD = (1.0f.xxx - kS) * (1.0f - surface.metalness);
            
            float3 baseAmbient = 0.03f.xxx;
            
            // 【重要】テクスチャのAOと、Tree.VSで計算した「擬似AO (input.color.a)」を合成する
            float combinedAO = lerp(gMaterial.shadowEnvStrength, 1.0f, shadowFactor) * input.color.a;

            float3 ambientDiffuse = kD * surface.pbrAlbedo * baseAmbient;

            // IBL
            float3 reflectionVector = reflect(-toEyeWorld, surface.normal);
            float3 envColor = gEnvironmentTexture.SampleLevel(gSampler, reflectionVector, surface.roughness * 6.0f).rgb;
            float3 F0 = lerp(0.04f.xxx, surface.pbrAlbedo, surface.metalness);
            float3 F_env = F_SchlickRoughness(max(dot(surface.normal, toEyeWorld), 0.0f), F0, surface.roughness);
            float3 ambientSpecular = envColor * F_env;

            float3 ambient = (ambientDiffuse + ambientSpecular) * gMaterial.environmentMapIntensity;
            
            // 合成したAOを適用
            finalColor += ambient * combinedAO;
        }
    }
    else
    {
        finalColor = surface.albedo;
    }

    // 7. G-Buffer Output
    output.color.rgb = finalColor;
    output.color.a = 1.0f; // 幹は不透明なので1.0固定
    output.normal = float4(normal, 1.0f);
    output.material = float4(surface.metalness, surface.roughness, 0.0f, 1.0f);
    
    return output;
}