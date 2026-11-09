#include "Common/Object3D.hlsli"
#include "Common/ShaderConstants.hlsli"
#include "Common/MathUtils.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);

cbuffer DirectionalLights : register(b1)
{
    DirectionalLight gDirectionalLights[MAX_DIRECTIONAL_LIGHTS];
};

ConstantBuffer<GrassMaterialData> gMaterial : register(b5);
ConstantBuffer<GlobalEnvironmentData> gEnvironmentData : register(b7);
ConstantBuffer<ShadowData> gShadowData : register(b8);

Texture2DArray<float> gShadowMapArray : register(t2);
SamplerComparisonState gShadowSampler : register(s1);

struct GrassPSInput
{
    float4 position : SV_POSITION;
    float heightRatio : TEXCOORD; // 0:根元 ~ 1:先端
    float3 normal : NORMAL;
    float3 worldPosition : WORLD_POSITION;
};

// 陰になる側にも光を回り込ませる量
static const float kWrapLighting = 0.5f;

float CalculateGrassShadow(float3 worldPosition, float3 lightDirection, float distanceToCamera);

PixelShaderOutput main(GrassPSInput input)
{
    PixelShaderOutput output;

    float3 toCamera = gFrameData.cameraWorldPosition - input.worldPosition;
    float distanceToCamera = length(toCamera);
    float3 toEye = toCamera / max(distanceToCamera, kEpsilon);

    // カメラのすぐ近くの草はディザで消す
    float nearFade = saturate((distanceToCamera - gMaterial.nearFadeMinDist) / max(gMaterial.nearFadeMaxDist - gMaterial.nearFadeMinDist, kEpsilon));
    clip(nearFade - InterleavedGradientNoise(input.position.xy));

    float heightRatio = input.heightRatio;

    float3 baseColor = lerp(gMaterial.rootColor, gMaterial.tipColor, heightRatio);

    float3 lightDirection = normalize(-gDirectionalLights[0].direction);
    float3 lightColor = gDirectionalLights[0].color.rgb * gDirectionalLights[0].intensity;

    // 両面描画なので、裏から見ている面は法線をカメラ側へ向け直す
    float3 bladeNormal = normalize(input.normal);
    if (dot(bladeNormal, toEye) < 0.0f)
    {
        bladeNormal = -bladeNormal;
    }

    // 上向きに寄せて、草原全体が地面と同じように光を受けるようにする
    float3 normal = normalize(lerp(bladeNormal, float3(0.0f, 1.0f, 0.0f), gMaterial.grassNormalBlend));

    // 影は面の向きではなく位置だけで決める。向きで決めると、日の当たらない側の草に木の影が乗らなくなる
    float shadow = CalculateGrassShadow(input.worldPosition, lightDirection, distanceToCamera);

    float normalDotLight = saturate((dot(normal, lightDirection) + kWrapLighting) / (1.0f + kWrapLighting));
    float3 diffuse = baseColor * lightColor * normalDotLight;

    // 逆光のとき、先端ほど光が透ける
    float translucencyAmount = pow(saturate(dot(-lightDirection, toEye + bladeNormal * 0.2f)), 2.5f) * heightRatio;
    float3 translucency = gMaterial.sssColor * translucencyAmount * gMaterial.sssStrength * gDirectionalLights[0].color.rgb;

    float3 directLight = (diffuse + translucency) * shadow;

    float skyLight = saturate(normal.y * 0.5f + 0.5f);
    float3 ambientColor = lerp(gEnvironmentData.ambientGroundColor.rgb, gEnvironmentData.ambientSkyColor.rgb, skyLight);
    float3 ambient = baseColor * ambientColor;

    output.color = float4(directLight + ambient, 1.0f);
    output.normal = float4(normal, 1.0f);
    return output;
}

// 草は数が多いので、1回だけサンプルする
float CalculateGrassShadow(float3 worldPosition, float3 lightDirection, float distanceToCamera)
{
    if (distanceToCamera > gShadowData.cascadeSplits[MAX_CASCADE_COUNT - 1])
        return 1.0f;

    float minShadow = 1.0f - saturate(gMaterial.shadowDensity);

    // 太陽が地平線より下
    if (lightDirection.y <= 0.0f)
        return minShadow;

    uint cascadeIndex = 0;
    [unroll]
    for (uint i = 0; i < MAX_CASCADE_COUNT - 1; ++i)
    {
        if (distanceToCamera > gShadowData.cascadeSplits[i])
        {
            cascadeIndex = i + 1;
        }
    }

    // 自分の影が乗らないように少し上へずらす。草は薄いので面の法線は使わない
    float biasScale = saturate(1.0f - lightDirection.y);
    float3 biasedPosition = worldPosition + float3(0.0f, gMaterial.shadowNormalBias * biasScale, 0.0f);

    float4 shadowClip = mul(float4(biasedPosition, 1.0f), gShadowData.cascadeLightViewProj[cascadeIndex]);
    float3 shadowPosition = shadowClip.xyz / shadowClip.w;
    float2 shadowUV = float2(shadowPosition.x * 0.5f + 0.5f, -shadowPosition.y * 0.5f + 0.5f);

    if (any(shadowUV < 0.0f) || any(shadowUV > 1.0f) || shadowPosition.z < 0.0f || shadowPosition.z > 1.0f)
    {
        return 1.0f;
    }

    float visibility = gShadowMapArray.SampleCmpLevelZero(
        gShadowSampler,
        float3(shadowUV, cascadeIndex),
        shadowPosition.z - gMaterial.shadowBias);

    return lerp(minShadow, 1.0f, visibility);
}
