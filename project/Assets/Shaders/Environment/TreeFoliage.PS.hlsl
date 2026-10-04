#include "Common/Object3D.hlsli"
#include "Common/ShaderConstants.hlsli"
#include "Common/MathUtils.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);
cbuffer DirectionalLights : register(b1)
{
    DirectionalLight gDirectionalLights[MAX_DIRECTIONAL_LIGHTS];
};
ConstantBuffer<GlobalEnvironmentData> gEnvironmentData : register(b4); 
ConstantBuffer<LeafMaterialData> gMaterial : register(b5);
ConstantBuffer<ShadowData> gShadowData : register(b8);

Texture2DArray<float> gShadowMapArray : register(t2);
Texture2D<float4> gAlbedoAlphaTex : register(t12);

SamplerComparisonState gShadowSampler : register(s1);
SamplerState gAnisoSampler : register(s3);

struct TreeFoliagePSInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 worldPosition : WORLD_POSITION;
    float4 color : COLOR0;
    float3 instanceTint : COLOR1;
    float lodFade : BLENDWEIGHT;
};

float CalculateFastShadowCSM(float3 worldPos, float3 normal, float viewDepth);

PixelShaderOutput main(TreeFoliagePSInput input, bool isFrontFace : SV_IsFrontFace)
{
    PixelShaderOutput output;

    // カメラからの距離を算出
    float3 cameraDiff = gFrameData.cameraWorldPosition - input.worldPosition;
    float viewDepth = length(cameraDiff);

    // カメラ近接フェード率の算出
    float proximityFade = saturate((viewDepth - gMaterial.nearFadeMinDist) / (gMaterial.nearFadeMaxDist - gMaterial.nearFadeMinDist));

    // LODクロスフェードと近接フェードの合成
    float finalFade = min(input.lodFade, proximityFade);

    // ディザリング判定
    float dither = InterleavedGradientNoise(input.position.xy);
    clip(finalFade - dither);

    float4 albedoAlpha = gAlbedoAlphaTex.Sample(gAnisoSampler, input.texcoord);
    
    // 完全透明なピクセルの破棄
    clip(albedoAlpha.a - 0.05f);

    albedoAlpha.rgb *= input.instanceTint * gMaterial.colorTint * max(gMaterial.albedoMultiplier, 0.0f);
    float ao = gMaterial.baseAO * input.color.a;

    // 天候パラメータからの動的Wetness反映
    albedoAlpha.rgb = lerp(albedoAlpha.rgb, albedoAlpha.rgb * 0.45f, gEnvironmentData.wetness);
    float roughness = lerp(gMaterial.baseRoughness, 0.05f, gEnvironmentData.wetness);

    // 両面ポリゴン向けに SV_IsFrontFace を用いて頂点法線を反転
    float faceSign = isFrontFace ? 1.0f : -1.0f;
    float3 normal = normalize(input.normal * faceSign);
    
    float3 toEye = cameraDiff / max(viewDepth, kEpsilon);
    float3 lightDir = normalize(-gDirectionalLights[0].direction);

    float shadowFactor = CalculateFastShadowCSM(input.worldPosition, normal, viewDepth);
    float3 attenuatedLight = (gDirectionalLights[0].color.rgb * gDirectionalLights[0].intensity) * shadowFactor;

    // Half-LambertライクなWrap
    float wrap = gMaterial.diffuseWrap;
    float NdotL = saturate((dot(normal, lightDir) + wrap) / ((1.0f + wrap) * (1.0f + wrap)));
    float3 diffuse = albedoAlpha.rgb * attenuatedLight * NdotL;

    // 逆光時の透過光
    float backLight = saturate(dot(-normal, lightDir));
    float sssIntensity = Pow5(backLight) * (1.0f - gMaterial.baseThickness) * gMaterial.sssStrength;
    float3 transmission = (albedoAlpha.rgb * gMaterial.sssColor) * attenuatedLight * sssIntensity;

    // Direct Specular
    float3 halfVector = normalize(lightDir + toEye);
    float NdotH = saturate(dot(normal, halfVector));
    float NdotV = saturate(dot(normal, toEye));
    
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    float denom = (NdotH * NdotH * (alpha2 - 1.0f) + 1.0f);
    float D = alpha2 / (PI * denom * denom + kEpsilon);

    float3 F0 = lerp(0.04f.xxx, 0.02f.xxx, gEnvironmentData.wetness);
    float3 F = F0 + (1.0f.xxx - F0) * Pow5(1.0f - saturate(dot(halfVector, toEye)));

    float gustMask = input.color.r;
    float3 directSpecular = (D * F) * attenuatedLight * NdotL * (1.0f + gustMask * 2.0f);

    // IBL Specular & Ambient
    float3 reflectDir = reflect(-toEye, normal);
    float skyWeight = saturate(reflectDir.y * 0.5f + 0.5f);
    float3 envSkyColor = lerp(gEnvironmentData.groundColor.rgb, gEnvironmentData.skyColor.rgb, skyWeight);
    envSkyColor *= (1.0f - roughness * 0.5f);

    float3 F_env = F0 + (max(1.0f.xxx - roughness, F0) - F0) * Pow5(1.0f - NdotV);
    float3 ambientSpecular = envSkyColor * F_env * ao;

    float skyLight = saturate(normal.y * 0.5f + 0.5f);
    float3 ambientDiffuse = albedoAlpha.rgb * lerp(gEnvironmentData.groundColor.rgb, gEnvironmentData.skyColor.rgb, skyLight) * ao;

    float3 finalColor = diffuse + transmission + directSpecular + ambientDiffuse + ambientSpecular;

    // Alpha-to-Coverage アンチエイリアス補正
    float outAlpha = (albedoAlpha.a - gMaterial.alphaCutoff) / max(fwidth(albedoAlpha.a), kEpsilon) + 0.5f;
    
    output.color = float4(finalColor, saturate(outAlpha));
    output.normal = float4(normal, 1.0f);
    output.material = float4(0.0f, roughness, 0.0f, 1.0f);

    return output;
}

float CalculateFastShadowCSM(float3 worldPos, float3 normal, float viewDepth)
{
    // 最遠カスケードを超えている場合は早期リターン
    if (viewDepth > gShadowData.cascadeSplits[MAX_CASCADE_COUNT - 1])
        return 1.0f;
    
    float3 lightDir = normalize(-gDirectionalLights[0].direction);
    float NdotL = dot(normal, lightDir);
    float minShadow = 1.0f - saturate(gMaterial.shadowDensity);

    // 光の裏側はサンプリングをスキップ
    if (NdotL <= 0.0f)
        return minShadow;

    // カスケードインデックスの決定
    uint cascadeIndex = 0;
    [unroll]
    for (uint i = 0; i < MAX_CASCADE_COUNT - 1; ++i)
    {
        if (viewDepth > gShadowData.cascadeSplits[i])
        {
            cascadeIndex = i + 1;
        }
    }
    
    // シャドウアクネ対策
    float biasScale = saturate(1.0f - NdotL);
    float3 biasedWorldPos = worldPos + normal * (gMaterial.shadowNormalBias * biasScale);

    // 行列変換とプロジェクション座標計算
    float4 shadowCoord = mul(float4(biasedWorldPos, 1.0f), gShadowData.cascadeLightViewProj[cascadeIndex]);
    float3 projCoords = shadowCoord.xyz / shadowCoord.w;

    projCoords.x = projCoords.x * 0.5f + 0.5f;
    projCoords.y = -projCoords.y * 0.5f + 0.5f;

    float currentDepth = projCoords.z - gMaterial.shadowBias;

    // フラスタム外チェック
    if (any(projCoords < 0.0f) || any(projCoords > 1.0f))
    {
        return 1.0f;
    }

    // PCF サンプリング
    float shadowVisibility = gShadowMapArray.SampleCmpLevelZero(
        gShadowSampler,
        float3(projCoords.xy, cascadeIndex),
        currentDepth
    );

    return lerp(minShadow, 1.0f, shadowVisibility);
}