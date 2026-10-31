#include "Common/Object3D.hlsli"
#include "Common/ShaderConstants.hlsli"
#include "Common/MathUtils.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);

cbuffer DirectionalLights : register(b1)
{
    DirectionalLight gDirectionalLights[MAX_DIRECTIONAL_LIGHTS];
};

ConstantBuffer<GlobalEnvironmentData> gEnvironmentData : register(b4);
ConstantBuffer<FoliageMaterialData> gMaterial : register(b5);
ConstantBuffer<ShadowData> gShadowData : register(b8);

Texture2D<float4> gAlbedoAlphaTex : register(t0);
Texture2DArray<float> gShadowMapArray : register(t1);

SamplerComparisonState gShadowSampler : register(s1);
SamplerState gAnisoSampler : register(s3);

struct FoliagePSInput
{
    float4 position : SV_POSITION;
    float3 worldPosition : WORLD_POSITION;
    float2 texcoord : TEXCOORD;
    float3 normal : NORMAL;
};

float CalculateShadow(float3 worldPosition, float3 normal, float viewDepth);

PixelShaderOutput main(FoliagePSInput input, bool isFrontFace : SV_IsFrontFace)
{
    float4 albedoAlpha = gAlbedoAlphaTex.Sample(gAnisoSampler, input.texcoord);
    float3 albedo = albedoAlpha.rgb * gMaterial.baseColor * gMaterial.colorIntensity;

    // 1 ピクセル分のアルファの変化量で割って、拡大しても輪郭がぼやけないように抜く
    float alpha = (albedoAlpha.a - gMaterial.alphaCutoff) / max(fwidth(albedoAlpha.a), kEpsilon) + 0.5f;
    clip(alpha - 0.5f);

    // 両面ポリゴンなので、裏面は法線を反転する
    float3 normal = normalize(input.normal * (isFrontFace ? 1.0f : -1.0f));

    float viewDepth = distance(gFrameData.cameraWorldPosition, input.worldPosition);
    float3 lightDirection = normalize(-gDirectionalLights[0].direction);
    float shadowFactor = CalculateShadow(input.worldPosition, normal, viewDepth);
    float3 lightColor = gDirectionalLights[0].color.rgb * gDirectionalLights[0].intensity * shadowFactor;

    // Half-Lambert で光の裏側も少し明るくする
    float3 diffuse = albedo * lightColor * saturate(dot(normal, lightDirection) * 0.5f + 0.5f);

    // 空と地面の色による環境光。影の中が真っ黒にならないようにする
    float skyWeight = saturate(normal.y * 0.5f + 0.5f);
    float3 ambient = albedo * lerp(gEnvironmentData.groundColor.rgb, gEnvironmentData.skyColor.rgb, skyWeight);

    PixelShaderOutput output;
    output.color = float4(diffuse + ambient, 1.0f);
    output.normal = float4(normal, 1.0f);
    return output;
}

float CalculateShadow(float3 worldPosition, float3 normal, float viewDepth)
{
    if (viewDepth > gShadowData.cascadeSplits[MAX_CASCADE_COUNT - 1])
    {
        return 1.0f;
    }

    float3 lightDirection = normalize(-gDirectionalLights[0].direction);
    float NdotL = dot(normal, lightDirection);
    float minShadow = 1.0f - saturate(gMaterial.shadowDensity);

    // 光の真裏だけは調べずに影にする (少し回り込む面は影で潰さない)
    if (NdotL <= -0.3f)
    {
        return minShadow;
    }

    uint cascadeIndex = 0;
    [unroll]
    for (uint splitIndex = 0; splitIndex < MAX_CASCADE_COUNT - 1; ++splitIndex)
    {
        if (viewDepth > gShadowData.cascadeSplits[splitIndex])
        {
            cascadeIndex = splitIndex + 1;
        }
    }

    // 光に対して斜めな面ほど法線方向にずらして、縞模様の影 (シャドウアクネ) を防ぐ
    float3 biasedPosition = worldPosition + normal * (gMaterial.shadowNormalBias * saturate(1.0f - NdotL));

    float4 shadowPosition = mul(float4(biasedPosition, 1.0f), gShadowData.cascadeLightViewProj[cascadeIndex]);
    float3 shadowCoord = shadowPosition.xyz / shadowPosition.w;
    shadowCoord.x = shadowCoord.x * 0.5f + 0.5f;
    shadowCoord.y = -shadowCoord.y * 0.5f + 0.5f;

    if (any(shadowCoord < 0.0f) || any(shadowCoord > 1.0f))
    {
        return 1.0f;
    }

    float visibility = gShadowMapArray.SampleCmpLevelZero(
        gShadowSampler, float3(shadowCoord.xy, cascadeIndex), shadowCoord.z - gMaterial.shadowBias);

    return lerp(minShadow, 1.0f, visibility);
}
