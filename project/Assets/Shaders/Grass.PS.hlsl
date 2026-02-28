#include "Object3D.hlsli"
#include "ShaderConstants.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);
cbuffer DirectionalLights : register(b1)
{
    DirectionalLight gDirectionalLights[MAX_DIRECTIONAL_LIGHTS];
};
ConstantBuffer<MaterialData> gMaterial : register(b5);

Texture2D<float4> gTexture : register(t0);
Texture2D<float> gShadowMap : register(t1);

SamplerState gSampler : register(s0);
SamplerComparisonState gShadowSampler : register(s1);

struct PixelInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
    float3 normal : NORMAL;
    float3 worldPosition : WORLD_POSITION;
    float4 color : COLOR;
    float4 shadowCoord : SHADOW_COORD;
};

// シャドウ強度を計算
float CalculateShadow(float4 shadowCoord, float3 normal);

PixelShaderOutput main(PixelInput input)
{
    PixelShaderOutput output;

    float4 textureColor = gTexture.Sample(gSampler, input.texcoord);

    if (textureColor.a < gMaterial.grassAlphaCutoff)
    {
        discard;
    }

    float3 baseColor = textureColor.rgb * gMaterial.color.rgb * input.color.rgb;
    float3 toEye = normalize(gFrameData.cameraWorldPosition - input.worldPosition);
    float3 lightDir = normalize(-gDirectionalLights[0].direction);
    float3 normal = normalize(input.normal);

    float shadowFactor = 1.0f;
    if (gMaterial.addShadow != 0)
    {
        shadowFactor = CalculateShadow(input.shadowCoord, normal);
    }

    float NdotL = dot(normal, lightDir) * 0.5f + 0.5f;
    float3 diffuse = baseColor * gDirectionalLights[0].color.rgb * gDirectionalLights[0].intensity * NdotL * shadowFactor;

    // 透過光
    float viewDotLight = saturate(dot(toEye, -lightDir));

    float3 translucency = baseColor * pow(viewDotLight, 3.0f) * gDirectionalLights[0].color.rgb * gMaterial.grassTranslucency * shadowFactor;

    float3 finalColor = diffuse + translucency + (baseColor * 0.2f); // 0.2fはAmbient

    finalColor *= smoothstep(1.0f, gMaterial.grassRootAO, input.texcoord.y);

    finalColor *= input.color.rgb;

    output.color = float4(finalColor, 1.0f);
    output.normal = float4(normal, 1.0f);
    output.material = float4(gMaterial.metalness, gMaterial.roughness, 0.0f, 1.0f);

    return output;
}

static const float2 poissonDisk[16] =
{
    float2(-0.94201624, -0.39906216),
    float2(0.94558609, -0.76890725),
    float2(-0.094184101, -0.92938870),
    float2(0.34495938, 0.29387760),
    float2(-0.91588581, 0.45771432),
    float2(-0.81544232, -0.87912464),
    float2(-0.38277543, 0.27676845),
    float2(0.97484398, 0.75648379),
    float2(0.44323325, -0.97511554),
    float2(0.53742981, -0.47373420),
    float2(-0.26496911, -0.41893023),
    float2(0.79197514, 0.19090188),
    float2(-0.24188840, 0.99706507),
    float2(-0.81409955, 0.91437590),
    float2(0.19984126, 0.78641367),
    float2(0.14383161, -0.14100790)
};

float CalculateShadow(float4 shadowCoord, float3 normal)
{
    float3 projCoords = shadowCoord.xyz / shadowCoord.w;

    // 法線ベースのバイアス
    float3 lightDir = normalize(-gDirectionalLights[0].direction);
    float biasScale = saturate(1.0f - dot(normal, lightDir));

    float depthBias = gMaterial.shadowBias;
    float normalBias = 0.002f * biasScale;

    // NDC→UV
    projCoords.x = projCoords.x * 0.5f + 0.5f;
    projCoords.y = -projCoords.y * 0.5f + 0.5f;

    // 法線オフセット
    projCoords.xy += normal.xy * normalBias;

    float currentDepth = projCoords.z - depthBias;

    // 範囲外
    if (projCoords.z < 0.0f || projCoords.z > 1.0f ||
        projCoords.x < 0.0f || projCoords.x > 1.0f ||
        projCoords.y < 0.0f || projCoords.y > 1.0f)
    {
        return 1.0f;
    }

    // PCF
    float2 texelSize = 1.0f / float2(2048.0f, 2048.0f);
    float softness = max(gMaterial.shadowSoftness, 1.0f);

    float shadow = 0.0f;
    [unroll]
    for (int i = 0; i < 16; ++i)
    {
        float2 offset = poissonDisk[i] * texelSize * softness;
        shadow += gShadowMap.SampleCmpLevelZero(
            gShadowSampler,
            projCoords.xy + offset,
            currentDepth
        );
    }

    // 平均化（0.0が完全な影、1.0が完全な光）
    float shadowVisibility = shadow * (1.0f / 16.0f);

    float densityLimit = min(gMaterial.shadowDensity, 0.99f);
    
    // densityLimitが高いほど、薄いグレーの影が黒(0.0)に変換され、影が太くくっきりする
    return smoothstep(densityLimit, 1.0f, shadowVisibility);
}
