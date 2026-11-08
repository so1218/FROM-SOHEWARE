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
ConstantBuffer<TerrainMaterialData> gMaterial : register(b5);
ConstantBuffer<ShadowData> gShadowData : register(b8);

Texture2D<float4> gAlbedoTexture : register(t0);
TextureCube<float4> gEnvironmentTexture : register(t1);
Texture2DArray<float> gShadowMapArray : register(t2);
Texture2D<float4> gNormalTexture : register(t3);
Texture2D<float4> gRippleTexture : register(t4);

SamplerState gSampler : register(s0);
SamplerComparisonState gShadowSampler : register(s1);

// 3枚のレイヤーをずらして重ね、雨粒の波紋の法線を作る
float3 CalculateRippleNormal(float3 worldPosition)
{
    float2 rippleUV = worldPosition.xz * gMaterial.rippleScale;
    float time = gFrameData.time * gMaterial.rippleSpeed;
    float3 combinedRipple = 0.0f.xxx;

    [unroll]
    for (int i = 0; i < 3; i++)
    {
        float2 p = rippleUV + float2(i * 0.33f, i * 0.71f);
        float2 cell = floor(p);
        float2 f = frac(p);

        // セルごとに発生タイミングをずらす
        float random = frac(sin(dot(cell + float(i), float2(12.9898f, 78.233f))) * 43758.5453f);
        float localTime = frac(time * gMaterial.rippleFrequency + random);

        float spread = localTime * gMaterial.rippleSize + 0.0001f;
        float2 animatedUV = (f - 0.5f) / spread + 0.5f;

        float3 ripple = 0.0f.xxx;
        if (all(animatedUV >= 0.0f) && all(animatedUV <= 1.0f))
        {
            ripple = gRippleTexture.Sample(gSampler, animatedUV).xyz * 2.0f - 1.0f;
        }

        float fade = smoothstep(1.0f, 0.0f, localTime);
        float edgeMask = smoothstep(0.5f, 0.4f, length(f - 0.5f));
        combinedRipple += ripple * fade * edgeMask;
    }

    return combinedRipple * gMaterial.rippleStrength * gMaterial.wetness;
}

PixelShaderOutput main(PixelShaderInput input)
{
    PixelShaderOutput output;

    float3 toEye = normalize(gFrameData.cameraWorldPosition - input.worldPosition);
    float3 worldNormal = normalize(input.normal);
    float3 baseColor = gAlbedoTexture.Sample(gSampler, input.texcoord).rgb;

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
        float2 normalUV = input.texcoord * gMaterial.normalTiling;
        normal = CalculateNormalFromMap(worldNormal, input.tangent, normalUV, gMaterial.normalIntensity, gNormalTexture, gSampler);
    }

    float roughness = clamp(gMaterial.roughness, 0.05f, 1.0f);

    // 濡れている分だけ表面を滑らかにし、波紋で法線を揺らす
    if (gMaterial.wetness > 0.0f)
    {
        roughness = lerp(roughness, 0.01f, gMaterial.wetness);
        float3 rippleNormal = CalculateRippleNormal(input.worldPosition);
        normal = normalize(normal + float3(rippleNormal.x, 0.0f, rippleNormal.y));
    }

    SurfaceData surface;
    surface.albedo = baseColor * gMaterial.color.rgb;
    surface.pbrAlbedo = baseColor * pow(abs(gMaterial.color.rgb), 2.2f);
    surface.specularColor = 0.0f.xxx;
    surface.normal = normal;
    surface.roughness = roughness;
    surface.metalness = saturate(gMaterial.metalness);
    surface.shininess = 1.0f;
    surface.diffuseReflection = 1.0f;
    surface.lightMode = SHADING_MODEL_PBR;

    float3 finalColor = ApplyDirectionalLightsPBR(surface, toEye, shadowFactor, gDirectionalLights);
    finalColor += ApplyPointLights(surface, input.worldPosition, toEye, gPointLights);
    finalColor += ApplySpotLights(surface, input.worldPosition, toEye, gSpotLights);
    finalColor += ApplyAmbient(surface, toEye, shadowFactor, gMaterial.shadowEnvStrength,
        gMaterial.environmentMapIntensity, gEnvironmentTexture, gSampler);
    finalColor *= gMaterial.emissiveIntensity;

    output.color = float4(finalColor, 1.0f);
    output.normal = float4(normal, 1.0f);
    return output;
}
