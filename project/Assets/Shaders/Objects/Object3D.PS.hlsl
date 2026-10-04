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
cbuffer AreaLightsBuffer : register(b4)
{
    AreaLight gAreaLights[MAX_AREA_LIGHTS];
};
ConstantBuffer<MaterialData> gMaterial : register(b5);
ConstantBuffer<ShadowData> gShadowData : register(b8);

Texture2D<float4> gTexture : register(t0);
TextureCube<float4> gEnvironmentTexture : register(t1);
Texture2DArray<float> gShadowMapArray : register(t2);
Texture2D<float4> gToonRamp : register(t3);
Texture2D<float4> gDissolveTexture : register(t4);
Texture2D<float4> gNormalTexture : register(t5);
Texture2D<float4> gRippleTexture : register(t6);

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
    // Debug: Art Grid
    // --------------------------------------------------------
    if (gMaterial.isArtGrid)
    {
        if (ShouldDiscardArtGrid(input.texcoord))
            discard;

        output.color = float4(DrawArtGridColor(input.texcoord, gFrameData.iResolution), 1.0f);
        
        output.normal = float4(0.0f, 1.0f, 0.0f, 1.0f);
        output.material = float4(0.0f, 1.0f, 0.0f, 1.0f);
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
    
    // --------------------------------------------------------
    // Wetness & Ripple (雨・波紋表現)
    // --------------------------------------------------------
    if (gMaterial.enableRipple != 0 && gMaterial.wetness > 0.0f)
    {
        float globalWetness = gMaterial.wetness;

        // 水濡れによるラフネスの平滑化
        currentRoughness = lerp(currentRoughness, 0.01f, globalWetness);

        // レイヤーのUVをずらしてサンプリングし、不規則な波紋アニメーションを作る
        float2 rippleUV = input.worldPosition.xz * gMaterial.rippleScale;
        float time = gFrameData.gTime * gMaterial.rippleSpeed;
        float3 combinedRipple = 0.0f.xxx;

        [unroll]
        for (int i = 0; i < 3; i++)
        {
            float2 offset = float2(i * 0.33f, i * 0.71f);
            float2 p = rippleUV + offset;
            float2 gridID = floor(p);
            float2 f = frac(p);

            // セルごとのランダムシード生成
            float3 seed = float3(gridID, float(i));
            float rand = frac(sin(dot(seed.xy + seed.z, float2(12.9898f, 78.233f))) * 43758.5453f);
            float localTime = frac(time * gMaterial.rippleFrequency + rand);

            float spread = localTime * gMaterial.rippleSize + 0.0001f;
            float2 animatedUV = (f - 0.5f) / spread + 0.5f;

            float3 r = 0.0f.xxx;
            if (animatedUV.x >= 0.0f && animatedUV.x <= 1.0f && animatedUV.y >= 0.0f && animatedUV.y <= 1.0f)
            {
                r = gRippleTexture.Sample(gSampler, animatedUV).xyz * 2.0f - 1.0f;
            }

            float mask = smoothstep(1.0f, 0.0f, localTime);
            float edgeMask = smoothstep(0.5f, 0.4f, length(f - 0.5f));
            combinedRipple += r * mask * edgeMask;
        }

        float3 rippleNormal = combinedRipple * gMaterial.rippleStrength * globalWetness;
        normal = normalize(normal + float3(rippleNormal.x, 0.0f, rippleNormal.y));
    }
    
    // --------------------------------------------------------
    // Bubble (薄膜干渉)
    // --------------------------------------------------------
    float3 toEye = normalize(gFrameData.cameraWorldPosition - input.worldPosition);
    float bubbleAlpha = textureColor.a;
    
    if (gMaterial.isBubble != 0)
    {
        float3 bubbleNormal = normalize(input.normal);
        float NdotV = saturate(dot(bubbleNormal, toEye));
        float fresnel = 1.0f - NdotV;

        float t = fresnel + gFrameData.gTime * (gMaterial.wobbleSpeed * 0.1f);
        float3 a = 0.5f.xxx;
        float3 b = 0.5f.xxx;
        float3 c = 1.0f.xxx;
        float3 d = float3(0.00f, 0.33f, 0.67f);
        float3 rainbowColor = a + b * cos(6.28318f * (c * t + d));

        baseColor += rainbowColor * fresnel * gMaterial.rainbowIntensity;
        bubbleAlpha = lerp(0.1f, 1.0f, pow(fresnel, gMaterial.fresnelExponent));
    }
    
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
        finalColor += ApplyAreaLights(surface, input.worldPosition, toEyeWorld, gAreaLights);

        if (gMaterial.lightMode == SHADING_MODEL_PBR)
        {
            float3 kS = F_SchlickRoughness(max(dot(surface.normal, toEyeWorld), 0.0f), 0.04f.xxx, surface.roughness);
            float3 kD = (1.0f.xxx - kS) * (1.0f - surface.metalness);
            
            float3 baseAmbient = 0.03f.xxx;
            float ambientOcclusion = lerp(gMaterial.shadowEnvStrength, 1.0f, shadowFactor);

            float3 ambientDiffuse = kD * surface.pbrAlbedo * baseAmbient;

            float3 reflectionVector = reflect(-toEyeWorld, surface.normal);
            float3 envColor = gEnvironmentTexture.SampleLevel(gSampler, reflectionVector, surface.roughness * 6.0f).rgb;
    
            float3 F0 = lerp(0.04f.xxx, surface.pbrAlbedo, surface.metalness);
            float3 F_env = F_SchlickRoughness(max(dot(surface.normal, toEyeWorld), 0.0f), F0, surface.roughness);
            float3 ambientSpecular = envColor * F_env;

            float3 ambient = (ambientDiffuse + ambientSpecular) * gMaterial.environmentMapIntensity;
            finalColor += ambient * ambientOcclusion;
        }
        else
        {
            float3 reflectionVector = reflect(-toEyeWorld, surface.normal);
            float4 envColor = gEnvironmentTexture.Sample(gSampler, reflectionVector);
            float ambientOcclusion = lerp(gMaterial.shadowEnvStrength, 1.0f, shadowFactor);

            finalColor += envColor.rgb * gMaterial.environmentMapIntensity * ambientOcclusion;
        }
        
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
    output.color.a = (gMaterial.isBubble != 0) ? (bubbleAlpha * gMaterial.color.a) : (textureColor.a * gMaterial.color.a);
    output.normal = float4(normal, 1.0f);
    output.material = float4(currentMetalness, currentRoughness, 0.0f, 1.0f);
    
    return output;
}
