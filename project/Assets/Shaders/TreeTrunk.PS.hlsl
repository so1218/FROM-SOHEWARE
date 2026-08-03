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
ConstantBuffer<TrunkMaterialData> gMaterial : register(b6); // 幹用のマテリアルデータ
ConstantBuffer<ShadowData> gShadowData : register(b8);

Texture2D<float4> gTexture : register(t0); // 幹のアルベド
TextureCube<float4> gEnvironmentTexture : register(t1);
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
    float3 bitangent : BITANGENT;
    float3 worldPosition : WORLD_POSITION;
    float4 color : COLOR0; // GustMask と 擬似AO をPSに渡すために使用
    float3 instanceTint : COLOR1;
    float lodFade : BLENDWEIGHT;
};

PixelShaderOutput main(PixelInput input)
{
    PixelShaderOutput output;
    
    // 1. LODディザリング
    float dither = frac(sin(dot(input.position.xy, float2(12.9898f, 78.233f))) * 43758.5453f);
    clip(input.lodFade - dither);

    // 2. テクスチャサンプリング
    float4 textureColor = gTexture.Sample(gSampler, input.texcoord);
    float3 baseColor = textureColor.rgb * input.instanceTint;

    // =========================================================================
    // ★ 濡れ (Wetness) による物理的変化
    // =========================================================================
    float wetness = gEnvironmentData.wetness;

    // ① アルベドの暗化 (Porosity: 幹は水を吸うため色が暗く・濃くなる)
    float porosity = 0.6f; // 水を吸い込む度合い
    float3 wetColor = baseColor * (1.0f - porosity * 0.5f);
    baseColor = lerp(baseColor, wetColor, wetness);

    // ② ラフネスの低下 (水膜によってツヤが出る)
    float baseRoughness = clamp(gMaterial.roughness, 0.05f, 1.0f);
    // 濡れきった幹はラフネスが0.15付近まで下がり、周囲の光を反射するようになる
    float wetRoughness = lerp(baseRoughness, 0.15f, wetness);
    // =========================================================================

    float3 worldNormal = normalize(input.normal);
    float3 toEyeWorld = normalize(gFrameData.cameraWorldPosition - input.worldPosition);

    // 3. Normal Map
    float3 normal = worldNormal;
    if (gMaterial.enableNormalMap != 0)
    {
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
    
    // ★ 計算した濡れラフネスを適用
    surface.roughness = wetRoughness;
    // ★ メタルネスは絶対にそのまま (水は非金属)
    surface.metalness = saturate(gMaterial.metalness);
    
    surface.shininess = gMaterial.shininess;
    surface.diffuseReflection = gMaterial.diffuseReflection;
    surface.lightMode = gMaterial.lightMode;

    // 6. Lighting & PBR
    float3 finalColor = 0.0f.xxx;
    if (gMaterial.enableLighting != 0)
    {
        // ★ gToonRamp と gClampSampler を渡すことでエラー解消
        finalColor += ApplyDirectionalLights(surface, toEyeWorld, shadowFactor, gDirectionalLights, gToonRamp, gClampSampler);
        finalColor += ApplyPointLights(surface, input.worldPosition, toEyeWorld, gPointLights);
        finalColor += ApplySpotLights(surface, input.worldPosition, toEyeWorld, gSpotLights);

        if (gMaterial.lightMode == SHADING_MODEL_PBR)
        {
            float3 kS = F_SchlickRoughness(max(dot(surface.normal, toEyeWorld), 0.0f), 0.04f.xxx, surface.roughness);
            float3 kD = (1.0f.xxx - kS) * (1.0f - surface.metalness);
            
            float3 baseAmbient = 0.03f.xxx;
            
            // テクスチャのAOと、Tree.VSで計算した「擬似AO (input.color.a)」を合成
            float combinedAO = lerp(gMaterial.shadowEnvStrength, 1.0f, shadowFactor) * input.color.a;

            float3 ambientDiffuse = kD * surface.pbrAlbedo * baseAmbient;

            // IBL (ここで下がったroughnessが使われるため、濡れた時に空の反射が強くなる)
            float3 reflectionVector = reflect(-toEyeWorld, surface.normal);
            float3 envColor = gEnvironmentTexture.SampleLevel(gSampler, reflectionVector, surface.roughness * 6.0f).rgb;
            
            // ★ 水のF0(0.02)を考慮 (濡れるとF0が水の値に近づく)
            float3 baseF0 = lerp(0.04f.xxx, surface.pbrAlbedo, surface.metalness);
            float3 F0 = lerp(baseF0, 0.02f.xxx, wetness);
            
            float3 F_env = F_SchlickRoughness(max(dot(surface.normal, toEyeWorld), 0.0f), F0, surface.roughness);
            float3 ambientSpecular = envColor * F_env;

            float3 ambient = (ambientDiffuse + ambientSpecular) * gMaterial.environmentMapIntensity;
            
            finalColor += ambient * combinedAO;
        }
    }
    else
    {
        finalColor = surface.albedo;
    }

    // 7. G-Buffer Output
    output.color.rgb = finalColor;
    output.color.a = 1.0f;
    output.normal = float4(normal, 1.0f);
    
    // G-Buffer等に書き出す際も、メタルネスは維持し、濡れラフネスを書き出す
    output.material = float4(surface.metalness, surface.roughness, 0.0f, 1.0f);
    output.velocity = float2(0.0f, 0.0f);
    
    return output;
}