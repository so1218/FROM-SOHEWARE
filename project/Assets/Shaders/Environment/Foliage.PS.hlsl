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
Texture2D<float3> gNormalTex : register(t1);
Texture2DArray<float> gShadowMapArray : register(t2);

SamplerComparisonState gShadowSampler : register(s1);
SamplerState gAnisoSampler : register(s3);

struct FoliagePSInput
{
    float4 position : SV_POSITION;
    float3 worldPosition : WORLD_POSITION;
    float2 texcoord : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float4 color : COLOR0;
    float3 instanceTint : COLOR1;
    float2 velocity : TEXCOORD1;
};

float CalculateFastShadowCSM(float3 worldPos, float3 normal, float viewDepth);

PixelShaderOutput main(FoliagePSInput input, bool isFrontFace : SV_IsFrontFace)
{
    PixelShaderOutput output;

    float4 albedoAlpha = gAlbedoAlphaTex.Sample(gAnisoSampler, input.texcoord);
    clip(albedoAlpha.a - 0.05f);

    float3 albedo = albedoAlpha.rgb * gMaterial.baseColor * input.instanceTint;
    
    // wetnessによる粗さの低下を削除（または影響度を微小化）して明るさを維持
    float currentRoughness = clamp(gMaterial.roughness, 0.3f, 1.0f);
    // 濡れツヤだけ少し出したい場合は影響度を 0.5f から 0.05f〜0.1f 程度に抑える
    // currentRoughness = lerp(currentRoughness, 0.3f, gEnvironmentData.wetness * 0.1f);

    float faceSign = isFrontFace ? 1.0f : -1.0f;
    float3 N = normalize(input.normal * faceSign);
    float3 T = normalize(input.tangent * faceSign);
    float3 B = cross(N, T);
    float3x3 TBN = float3x3(T, B, N);
    
    float3 tangentNormal = gNormalTex.Sample(gAnisoSampler, input.texcoord).xyz * 2.0f - 1.0f;
    float3 worldNormal = normalize(mul(tangentNormal, TBN));

    float3 cameraDiff = gFrameData.cameraWorldPosition - input.worldPosition;
    float viewDepth = length(cameraDiff);
    float3 toEye = cameraDiff / max(viewDepth, kEpsilon);
    float3 lightDir = normalize(-gDirectionalLights[0].direction);
    
    // 影の計算
    float shadowFactor = CalculateFastShadowCSM(input.worldPosition, worldNormal, viewDepth);
    float3 attenuatedLight = gDirectionalLights[0].color.rgb * gDirectionalLights[0].intensity * shadowFactor;

    // Wrap Diffuse (葉の光の回り込み)
    float wrap = 0.3f;
    float NdotL = saturate((dot(worldNormal, lightDir) + wrap) / ((1.0f + wrap) * (1.0f + wrap)));
    float3 diffuse = albedo * attenuatedLight * NdotL;

    // 透過光
    float backLight = saturate(dot(-worldNormal, lightDir));
    float sssIntensity = Pow5(backLight) * gMaterial.sssStrength;
    float3 transmission = (albedo * 1.5f) * attenuatedLight * sssIntensity;

    // スペキュラ (GGX近似)
    float3 halfVector = normalize(lightDir + toEye);
    float NdotH = saturate(dot(worldNormal, halfVector));
    float NdotV = saturate(dot(worldNormal, toEye));
    
    float alpha = currentRoughness * currentRoughness;
    float alpha2 = alpha * alpha;
    float denom = (NdotH * NdotH * (alpha2 - 1.0f) + 1.0f);
    float D = alpha2 / (PI * denom * denom + kEpsilon);

    float3 F0 = float3(0.04f, 0.04f, 0.04f);
    float3 F = F0 + (1.0f - F0) * Pow5(1.0f - saturate(dot(halfVector, toEye)));
    float3 directSpecular = (D * F) * attenuatedLight * NdotL;

    // 環境光 
    float skyLight = saturate(worldNormal.y * 0.5f + 0.5f);
    float3 ambientDiffuse = albedo * gEnvironmentData.skyColor.rgb * skyLight * 0.8f;

    float3 F_env = F0 + (max(1.0f - currentRoughness, F0) - F0) * Pow5(1.0f - NdotV);
    float3 ambientSpecular = gEnvironmentData.skyColor.rgb * F_env * skyLight * 0.1f;

    float3 finalColor = diffuse + transmission + directSpecular + ambientDiffuse + ambientSpecular;

    float outAlpha = (albedoAlpha.a - gMaterial.alphaCutoff) / max(fwidth(albedoAlpha.a), kEpsilon) + 0.5f;
    
    output.color = float4(finalColor, saturate(outAlpha));
    output.normal = float4(worldNormal, 1.0f);
    output.material = float4(0.0f, currentRoughness, 0.0f, 1.0f);
    output.velocity = input.velocity;

    return output;
}

float CalculateFastShadowCSM(float3 worldPos, float3 normal, float viewDepth)
{
    if (viewDepth > gShadowData.cascadeSplits[MAX_CASCADE_COUNT - 1])
        return 1.0f;
    
    float3 lightDir = normalize(-gDirectionalLights[0].direction);
    float NdotL = dot(normal, lightDir);
    float minShadow = 1.0f - saturate(gMaterial.shadowDensity);

    // Wrap Diffuse の係数 (-0.3) と一致させることで、光の回り込み部分が影で潰れないようにする
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