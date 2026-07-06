#include "Object3D.hlsli"
#include "ShaderConstants.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);
cbuffer DirectionalLights : register(b1)
{
    DirectionalLight gDirectionalLights[MAX_DIRECTIONAL_LIGHTS];
};
ConstantBuffer<MaterialData> gMaterial : register(b5);
ConstantBuffer<ShadowData> gShadowData : register(b8);

Texture2D<float4> gTexture : register(t0);
Texture2DArray<float> gShadowMapArray : register(t2);

SamplerState gSampler : register(s0);
SamplerComparisonState gShadowSampler : register(s1);

struct PixelInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
    float3 normal : NORMAL;
    float3 worldPosition : WORLD_POSITION;
    float4 color : COLOR;
    float4 currentClipPos : POSITION1; 
    float4 prevClipPos : POSITION2;
};

// シャドウ強度を計算
float CalculateShadowCSM(float3 worldPos, float3 normal, float viewDepth);

PixelShaderOutput main(PixelInput input)
{
    PixelShaderOutput output;
    
    // テクスチャは純粋なカラーグラデーションとして使用
    float4 textureColor = gTexture.Sample(gSampler, input.texcoord);

    float3 baseColor = textureColor.rgb * gMaterial.color.rgb * input.color.rgb;
    float3 toEye = normalize(gFrameData.cameraWorldPosition - input.worldPosition);
    
    // 両面描画対策（法線の反転をDiffuse計算の前に行う）
    float3 normal = normalize(input.normal);
    if (dot(normal, toEye) < 0.0f)
    {
        normal = -normal;
    }

    float3 lightDir = normalize(-gDirectionalLights[0].direction);

    float shadowFactor = 1.0f;
    if (gMaterial.addShadow != 0)
    {
        // カメラからの距離を測って CSM の関数を呼ぶ
        float viewDepth = distance(gFrameData.cameraWorldPosition, input.worldPosition);
        shadowFactor = CalculateShadowCSM(input.worldPosition, normal, viewDepth);
    }
    
    // 雷フラッシュの計算
    float flashIntensity = gFrameData.lightningFlashIntensity;
    float3 flashColor = gFrameData.lightningFlashColor * flashIntensity;
    float flashShadowCancel = saturate(flashIntensity);
    shadowFactor = lerp(shadowFactor, 1.0f, flashShadowCancel);

    // 光の計算
    float NdotL = dot(normal, lightDir) * 0.5f + 0.5f; // ハーフランバートで柔らかく
    float3 diffuse = baseColor * gDirectionalLights[0].color.rgb * gDirectionalLights[0].intensity * NdotL * shadowFactor;

    // 透過光
    float viewDotLight = saturate(dot(toEye, -lightDir));
    float3 translucency = baseColor * pow(viewDotLight, 3.0f) * gDirectionalLights[0].color.rgb * gMaterial.grassTranslucency * shadowFactor;
 
    float3 ambient = baseColor * (0.2f + flashColor);
    
    float3 finalColor = diffuse + translucency + ambient;

    // 根本の影を適用（V座標をそのまま利用）
    finalColor *= smoothstep(1.0f, gMaterial.grassRootAO, input.texcoord.y);

    // 濡れたときのハイライト計算
    float3 specular = float3(0.0f, 0.0f, 0.0f);
    if (gMaterial.wetness > 0.0f)
    {
        float3 fakeLightDir = normalize(toEye + float3(0.0f, 0.5f, 0.0f));
        float3 H = normalize(fakeLightDir + toEye);
        
        // すでに法線は反転済みなのでそのまま使用
        float3 wetNormal = normalize(normal + float3(0.0f, 0.3f, 0.0f));
        float NdotH = saturate(dot(wetNormal, H));
        
        float shininess = lerp(30.0f, 150.0f, gMaterial.wetness);
        float specIntensity = pow(NdotH, shininess) * gMaterial.wetness;
        float shadowMask = lerp(0.3f, 1.0f, shadowFactor);
        
        specular = gDirectionalLights[0].color.rgb * specIntensity * gDirectionalLights[0].intensity * shadowMask;
    }

    finalColor += specular;

    output.color = float4(finalColor, 1.0f);
    output.normal = float4(normal, 1.0f);
    output.material = float4(gMaterial.metalness, gMaterial.roughness, 0.0f, 1.0f);

    // NDC（正規化デバイス座標）に変換して差分を計算
    float2 ndcCurrent = input.currentClipPos.xy / input.currentClipPos.w;
    float2 ndcPrev = input.prevClipPos.xy / input.prevClipPos.w;

    output.velocity = (ndcCurrent - ndcPrev) * float2(0.5f, -0.5f);

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

float CalculateShadowCSM(float3 worldPos, float3 normal, float viewDepth)
{
    float3 lightDir = normalize(-gDirectionalLights[0].direction);
    float NdotL = dot(normal, lightDir);
    float minShadow = 1.0f - saturate(gMaterial.shadowDensity);

    if (NdotL <= 0.0f)
    {
        return minShadow;
    }

    uint cascadeIndex = 0;
    if (viewDepth > gShadowData.cascadeSplits.x)
        cascadeIndex = 1;
    if (viewDepth > gShadowData.cascadeSplits.y)
        cascadeIndex = 2;
    if (viewDepth > gShadowData.cascadeSplits.z)
        cascadeIndex = 3;

    float biasScale = saturate(1.0f - NdotL);
    float worldNormalBias = gMaterial.shadowNormalBias * biasScale;
    float3 biasedWorldPos = worldPos + normal * worldNormalBias;

    float4 shadowCoord = mul(float4(biasedWorldPos, 1.0f), gShadowData.cascadeLightViewProj[cascadeIndex]);
    float3 projCoords = shadowCoord.xyz / shadowCoord.w;

    projCoords.x = projCoords.x * 0.5f + 0.5f;
    projCoords.y = -projCoords.y * 0.5f + 0.5f;

    float currentDepth = projCoords.z - gMaterial.shadowBias;

    if (projCoords.z < 0.0f || projCoords.z > 1.0f ||
        projCoords.x < 0.0f || projCoords.x > 1.0f ||
        projCoords.y < 0.0f || projCoords.y > 1.0f)
    {
        return 1.0f;
    }

    float2 texelSize = 1.0f / 2048.0f;
    float softness = max(gMaterial.shadowSoftness, 1.0f);
    float shadow = 0.0f;

    [unroll]
    for (int i = 0; i < 16; ++i)
    {
        float2 offset = poissonDisk[i] * texelSize * softness;
        shadow += gShadowMapArray.SampleCmpLevelZero(
            gShadowSampler,
            float3(projCoords.xy + offset, cascadeIndex),
            currentDepth
        );
    }

    float shadowVisibility = shadow * (1.0f / 16.0f);
    return lerp(minShadow, 1.0f, shadowVisibility);
}