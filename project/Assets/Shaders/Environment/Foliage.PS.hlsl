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
    float4 color : COLOR0;
    float3 instanceTint : COLOR1;
};

float CalculateFastShadowCSM(float3 worldPos, float3 normal, float viewDepth);

PixelShaderOutput main(FoliagePSInput input, bool isFrontFace : SV_IsFrontFace)
{
    PixelShaderOutput output;

    // アルベド ＆ アルファテスト
    float4 albedoAlpha = gAlbedoAlphaTex.Sample(gAnisoSampler, input.texcoord);
    float3 albedo = albedoAlpha.rgb * gMaterial.baseColor * input.instanceTint;

    // アルファの輪郭を滑らかに抜く処理
    float alpha = (albedoAlpha.a - gMaterial.alphaCutoff) / max(fwidth(albedoAlpha.a), kEpsilon) + 0.5f;
    clip(alpha - 0.5f);

    // 両面ポリゴン対応の法線補正
    float faceSign = isFrontFace ? 1.0f : -1.0f;
    float3 worldNormal = normalize(input.normal * faceSign);

    // 接地感の表現
    float rootAO = saturate(input.texcoord.y);
    float groundAO = lerp(0.2f, 1.0f, rootAO);

    // ベクトル ＆ 影の計算
    float3 cameraDiff = gFrameData.cameraWorldPosition - input.worldPosition;
    float viewDepth = length(cameraDiff);
    float3 L = normalize(-gDirectionalLights[0].direction);

    float shadowFactor = CalculateFastShadowCSM(input.worldPosition, worldNormal, viewDepth);
    float3 lightColor = gDirectionalLights[0].color.rgb * gDirectionalLights[0].intensity * shadowFactor;

    // ディフューズ ＆ 透過光 
    float NdotL = dot(worldNormal, L);
    float directDiffuseFactor = saturate(NdotL);
    float3 directDiffuse = albedo * lightColor * directDiffuseFactor;

    // 裏面の透過光
    float backLight = saturate(-NdotL);
    float3 transmission = albedo * lightColor * (backLight * gMaterial.sssStrength);

    // 環境光
    float skyFactor = worldNormal.y * 0.5f + 0.5f;
    float3 skyLighting = lerp(gEnvironmentData.groundColor.rgb, gEnvironmentData.skyColor.rgb, skyFactor);
    float3 ambientDiffuse = albedo * skyLighting * groundAO;

    // 最終カラー合成
    float3 finalColor = (directDiffuse + transmission) * groundAO + ambientDiffuse;

    output.color = float4(finalColor, 1.0f);
    output.normal = float4(worldNormal, 1.0f);
    output.material = float4(0.0f, 1.0f, 0.0f, 1.0f); 

    return output;
}

float CalculateFastShadowCSM(float3 worldPos, float3 normal, float viewDepth)
{
    if (viewDepth > gShadowData.cascadeSplits[MAX_CASCADE_COUNT - 1])
        return 1.0f;
    
    float3 lightDir = normalize(-gDirectionalLights[0].direction);
    float NdotL = dot(normal, lightDir);
    float minShadow = 1.0f - saturate(gMaterial.shadowDensity);

    // 光の回り込み部分が影で潰れないように
    if (NdotL <= -0.3f)
        return minShadow;

    uint cascadeIndex = 0;
    [unroll]
    for (uint i = 0; i < MAX_CASCADE_COUNT - 1; ++i)
    {
        if (viewDepth > gShadowData.cascadeSplits[i])
        {
            cascadeIndex = i + 1;
        }
    }
    
    float biasScale = saturate(1.0f - NdotL);
    float3 biasedWorldPos = worldPos + normal * (gMaterial.shadowNormalBias * biasScale);

    float4 shadowCoord = mul(float4(biasedWorldPos, 1.0f), gShadowData.cascadeLightViewProj[cascadeIndex]);
    float3 projCoords = shadowCoord.xyz / shadowCoord.w;

    projCoords.x = projCoords.x * 0.5f + 0.5f;
    projCoords.y = -projCoords.y * 0.5f + 0.5f;

    float currentDepth = projCoords.z - gMaterial.shadowBias;

    if (any(projCoords < 0.0f) || any(projCoords > 1.0f))
    {
        return 1.0f;
    }

    float shadowVisibility = gShadowMapArray.SampleCmpLevelZero(
        gShadowSampler,
        float3(projCoords.xy, cascadeIndex),
        currentDepth
    );

    return lerp(minShadow, 1.0f, shadowVisibility);
}