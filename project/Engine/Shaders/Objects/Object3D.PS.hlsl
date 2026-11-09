#include "Common/Object3D.hlsli"
#include "Common/ShaderConstants.hlsli"
#include "Common/GridUtils.hlsli"
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
ConstantBuffer<MaterialData> gMaterial : register(b5);
ConstantBuffer<ShadowData> gShadowData : register(b8);

Texture2D<float4> gTexture : register(t0);
TextureCube<float4> gEnvironmentTexture : register(t1);
Texture2DArray<float> gShadowMapArray : register(t2);
Texture2D<float4> gToonRamp : register(t3);
Texture2D<float4> gDissolveTexture : register(t4);
Texture2D<float4> gNormalTexture : register(t5);

SamplerState gSampler : register(s0);
SamplerComparisonState gShadowSampler : register(s1);
SamplerState gClampSampler : register(s2);

PixelShaderOutput main(PixelShaderInput input)
{
    PixelShaderOutput output;
    
    float3 toEyeWorld = normalize(gFrameData.cameraWorldPosition - input.worldPosition);
    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float2 finalUV = transformedUV.xy;
    float3 normalizedInputNormal = normalize(input.normal);
    
    // --------------------------------------------------------
    // Albedo & Alpha Test
    // --------------------------------------------------------
    float3 worldNormal = normalizedInputNormal;
    float4 textureColor = gTexture.Sample(gSampler, finalUV);
        
    // 早期カリング
    if (textureColor.a * gMaterial.color.a <= gMaterial.alphaTestThreshold)
        discard;
    
    float3 baseColor = textureColor.rgb;

    // --------------------------------------------------------
    // Dissolve エフェクト
    // --------------------------------------------------------
    float3 dissolveEdgeEmission = 0.0f.xxx;
    if (gMaterial.enableDissolve != 0)
    {
        float noiseValue = gDissolveTexture.Sample(gSampler, finalUV).r;
        if (noiseValue <= gMaterial.dissolveThreshold)
            discard;

        float difference = noiseValue - gMaterial.dissolveThreshold;
        if (difference < gMaterial.edgeWidth)
        {
            float t = smoothstep(0.0f, 1.0f, 1.0f - (difference / gMaterial.edgeWidth));
            dissolveEdgeEmission = gMaterial.edgeColor * t * gMaterial.edgeIntensity;
        }
    }

    // --------------------------------------------------------
    // Debug Grid
    // --------------------------------------------------------
    if (gMaterial.isArtGrid)
    {
        if (ShouldDiscardArtGrid(input.texcoord))
            discard;

        output.color = float4(DrawArtGridColor(input.texcoord, gFrameData.iResolution), 1.0f);
        
        output.normal = float4(0.0f, 1.0f, 0.0f, 1.0f);
        return output;
    }
    
    // --------------------------------------------------------
    // Shadow & Normal
    // --------------------------------------------------------
    float shadowFactor = 1.0f;
    if (gDirectionalLights[0].enable && gMaterial.addShadow != 0)
    {
        float viewDepth = distance(gFrameData.cameraWorldPosition, input.worldPosition);
        float3 lightDir = normalize(-gDirectionalLights[0].direction);
        shadowFactor = CalculateShadowCSM(input.worldPosition, worldNormal, viewDepth, lightDir,
            gMaterial.shadowDensity, gShadowData.cascadeSplits,
            gMaterial.shadowNormalBias, gMaterial.shadowBias, gMaterial.shadowSoftness,
            gShadowData.cascadeLightViewProj,
            gShadowMapArray,
            gShadowSampler);
    }
    
    float3 normal = worldNormal;
    if (gMaterial.enableNormalMap != 0)
    {
        float2 normalUV = finalUV * gMaterial.normalTiling;
        normal = CalculateNormalFromMap(worldNormal, input.tangent, normalUV, gMaterial.normalIntensity, gNormalTexture, gSampler);
    }
    
    float currentRoughness = clamp(gMaterial.roughness, 0.05f, 1.0f);
    float currentMetalness = saturate(gMaterial.metalness);
    
    // SurfaceData の構築
    SurfaceData surface;
    surface.albedo = baseColor * gMaterial.color.rgb;
    surface.pbrAlbedo = baseColor * pow(abs(gMaterial.color.rgb), 2.2f);
    surface.specularColor = gMaterial.specularColor.rgb;
    surface.normal = normal;
    surface.roughness = currentRoughness;
    surface.metalness = currentMetalness;
    surface.shininess = gMaterial.shininess;
    surface.diffuseReflection = gMaterial.diffuseReflection;
    surface.lightMode = gMaterial.lightMode;

    // --------------------------------------------------------
    // Lighting & IBL
    // --------------------------------------------------------
    float3 finalColor = 0.0f.xxx;
    
    if (gMaterial.enableLighting != 0)
    {
        finalColor += ApplyDirectionalLights(surface, toEyeWorld, shadowFactor, gDirectionalLights, gToonRamp, gClampSampler);
        finalColor += ApplyPointLights(surface, input.worldPosition, toEyeWorld, gPointLights);
        finalColor += ApplySpotLights(surface, input.worldPosition, toEyeWorld, gSpotLights);

        finalColor += ApplyAmbient(surface, toEyeWorld, shadowFactor, gMaterial.shadowEnvStrength,
            gMaterial.environmentMapIntensity, gEnvironmentTexture, gSampler);
        
        if (gMaterial.enableRim != 0)
        {
            float3 toLight = float3(0.0f, 1.0f, 0.0f);
            if (gDirectionalLights[0].enable != 0)
                toLight = normalize(-gDirectionalLights[0].direction);
            
            finalColor += ApplyRimLight(
                surface.normal, toEyeWorld, toLight,
                gMaterial.rimPower, gMaterial.rimUseLightDir,
                gMaterial.rimColor, gMaterial.rimIntensity);
        }
    }
    else
    {
        finalColor = surface.albedo;
    }
    
    finalColor *= input.worldColor.rgb;
    finalColor *= gMaterial.emissiveIntensity;
    finalColor += dissolveEdgeEmission;

    // --------------------------------------------------------
    // G-Buffer Output
    // --------------------------------------------------------
    output.color.rgb = finalColor;
    output.color.a = textureColor.a * gMaterial.color.a;
    output.normal = float4(normal, 1.0f);
    
    return output;
}
