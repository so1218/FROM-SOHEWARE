#include "Object3D.hlsli"
#include "ShaderConstants.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);
cbuffer DirectionalLights : register(b1)
{
    DirectionalLight gDirectionalLights[MAX_DIRECTIONAL_LIGHTS];
};
ConstantBuffer<GlobalEnvironmentData> gEnvironmentData : register(b4); // ★ 追加：Wetness等が入った環境バッファ
ConstantBuffer<LeafMaterialData> gMaterial : register(b5);
ConstantBuffer<ShadowData> gShadowData : register(b8);

Texture2DArray<float> gShadowMapArray : register(t2);
Texture2D<float4> gAlbedoAlphaTex : register(t12);
Texture2D<float3> gNormalTex : register(t13);
Texture2D<float4> gMetallicRoughnessTex : register(t14);
TextureCube<float4> gEnvironmentTexture : register(t1); // ★ 追加：IBL用環境マップ

SamplerComparisonState gShadowSampler : register(s1);
SamplerState gAnisoSampler : register(s3);

struct PixelInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
    float3 worldPosition : WORLD_POSITION;
    float4 color : COLOR0;
    float3 instanceTint : COLOR1;
    float lodFade : BLENDWEIGHT;
};

float CalculateFastShadowCSM(float3 worldPos, float3 normal, float viewDepth);

PixelShaderOutput main(PixelInput input, bool isFrontFace : SV_IsFrontFace)
{
    PixelShaderOutput output;
    
    // 1. LODディザリング
    float dither = frac(sin(dot(input.position.xy, float2(12.9898f, 78.233f))) * 43758.5453f);
    clip(input.lodFade - dither);

    // 2. テクスチャ取得
    float4 albedoAlpha = gAlbedoAlphaTex.Sample(gAnisoSampler, input.texcoord);
    clip(albedoAlpha.a - 0.05f);
    albedoAlpha.rgb *= input.instanceTint * gMaterial.colorTint * max(gMaterial.albedoMultiplier, 0.0f);

    float4 mrTex = gMetallicRoughnessTex.Sample(gAnisoSampler, input.texcoord);
    float baseRoughness = mrTex.g * gMaterial.roughnessScale;
    float texAO = (mrTex.r > 0.001f) ? mrTex.r : 1.0f;
    float ao = texAO * gMaterial.baseAO * input.color.a;

    // -------------------------------------------------------------------------
    // ★【RDR2 / Tsushima 級】濡れ (Wetness) による物理プロパティ変化
    // -------------------------------------------------------------------------
    // ① Porosity (多孔性): 水が染み込むと吸光され、ベースカラーが暗く鮮やかになる
    float3 wetAlbedo = albedoAlpha.rgb * 0.45f;
    albedoAlpha.rgb = lerp(albedoAlpha.rgb, wetAlbedo, gEnvironmentData.wetness);

    // ② Roughness: 濡れると水膜で超平滑（0.03～0.08）になる。これによってハイライトが激変！
    float roughness = lerp(baseRoughness, 0.05f, gEnvironmentData.wetness);

    // 法線計算
    float3 N = normalize(input.normal);
    float3 T = normalize(input.tangent);
    float3 B = normalize(input.bitangent);
    if (!isFrontFace)
    {
        N = -N;
        T = -T;
        B = -B;
        // ゼロベクトル化を防ぎつつ上向き(input.normal)に補正する
        float3 flatN = lerp(N, input.normal, gMaterial.backfaceFlatten);
        float len = length(flatN);
        N = (len > 0.0001f) ? flatN / len : input.normal;
    }
    float3x3 TBN = float3x3(T, B, N);
    float3 tangentNormal = gNormalTex.Sample(gAnisoSampler, input.texcoord).xyz * 2.0f - 1.0f;
    float3 normal = normalize(mul(tangentNormal, TBN));

    // ライティング基本ベクトル
    float3 toEye = normalize(gFrameData.cameraWorldPosition - input.worldPosition);
    float3 lightDir = normalize(-gDirectionalLights[0].direction);
    float3 lightColor = gDirectionalLights[0].color.rgb * gDirectionalLights[0].intensity;
    float viewDepth = distance(gFrameData.cameraWorldPosition, input.worldPosition);
    float shadowFactor = CalculateFastShadowCSM(input.worldPosition, normal, viewDepth);

    // --- Diffuse & Transmission ---
    float wrap = gMaterial.diffuseWrap;
    float NdotL = saturate((dot(normal, lightDir) + wrap) / ((1.0f + wrap) * (1.0f + wrap)));
    float3 diffuse = albedoAlpha.rgb * lightColor * NdotL * shadowFactor;

    // 透過光 (SSS)
    float3 hTransmission = normalize(lightDir + normal * gMaterial.transmissionDistortion);
    float VdotH_trans = saturate(dot(toEye, -hTransmission));
    float sssIntensity = pow(VdotH_trans, gMaterial.transmissionPower) * (1.0f - gMaterial.baseThickness) * gMaterial.sssStrength;
    float3 transmission = (albedoAlpha.rgb * gMaterial.sssColor) * lightColor * sssIntensity * shadowFactor;

    // -------------------------------------------------------------------------
    // ★【完全PBR仕様】太陽光によるハイライト (Direct Specular)
    // -------------------------------------------------------------------------
    float3 halfVector = normalize(lightDir + toEye);
    float NdotH = saturate(dot(normal, halfVector));
    float NdotV = saturate(dot(normal, toEye));
    
    // GGX NDF 計算
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    float denom = (NdotH * NdotH * (alpha2 - 1.0f) + 1.0f);
    float D = alpha2 / (3.14159265f * denom * denom + 0.00001f);

    // Fresnel (Schlick) : 水のF0 = 0.02, 葉のF0 = 0.04。濡れ具合で過渡
    float3 F0 = lerp(0.04f.xxx, 0.02f.xxx, gEnvironmentData.wetness);
    float3 F = F0 + (1.0f.xxx - F0) * pow(1.0f - saturate(dot(halfVector, toEye)), 5.0f);

    // 鏡面反射 (0.1fの固定乗算を排除し、物理的に正しい輝度を出す)
    float gustMask = input.color.r;
    float3 directSpecular = (D * F) * lightColor * NdotL * shadowFactor * (1.0f + gustMask * 2.0f);

    // -------------------------------------------------------------------------
    // ★【雨の日に最も重要な光】環境マップからの鏡面反射 (IBL Specular)
    // -------------------------------------------------------------------------
    // 雨の日は太陽光が弱く雲で覆われるため、空全体の反射（IBL）が濡れ感のキーになります
    float3 reflectDir = reflect(-toEye, normal);
    // Roughnessに応じてMipMapレベルを選択サンプリング
    float3 envSkyColor = gEnvironmentTexture.SampleLevel(gAnisoSampler, reflectDir, roughness * 6.0f).rgb;
    
    // フレネル(環境光用)
    float3 F_env = F0 + (max(1.0f.xxx - roughness, F0) - F0) * pow(1.0f - NdotV, 5.0f);
    float3 ambientSpecular = envSkyColor * F_env * ao;

    // Ambient Diffuse
    float skyLight = saturate(normal.y * 0.5f + 0.5f);
    float3 ambientDiffuse = albedoAlpha.rgb * (0.1f + skyLight * 0.25f) * ao;

    // -------------------------------------------------------------------------
    // 最終カラー合成
    // -------------------------------------------------------------------------
    float3 finalColor = diffuse + transmission + directSpecular + ambientDiffuse + ambientSpecular;

    // Alpha-to-Coverage
    float outAlpha = (albedoAlpha.a - gMaterial.alphaCutoff) / max(fwidth(albedoAlpha.a), 0.0001f) + 0.5f;
    
    output.color = float4(finalColor, saturate(outAlpha));
    output.normal = float4(normal, 1.0f);
    output.material = float4(0.0f, roughness, 0.0f, 1.0f); // Metalnessは常に0.0！
    output.velocity = float2(0.0f, 0.0f);

    return output;
}

// -----------------------------------------------------------------------------
// Foliage向け 軽量CSMフェッチ
// -----------------------------------------------------------------------------
// 膨大なピクセル面積を占める草描画の帯域幅を節約するため、1-Tap PCFで済ませる
float CalculateFastShadowCSM(float3 worldPos, float3 normal, float viewDepth)
{
    float3 lightDir = normalize(-gDirectionalLights[0].direction);
    float NdotL = dot(normal, lightDir);
    float minShadow = 1.0f - saturate(gMaterial.shadowDensity);

    // [Early-out] 光の裏側（セルフシャドウ領域）はテクスチャフェッチ自体をスキップ
    if (NdotL <= 0.0f)
        return minShadow;

    // CSM カスケード選択
    uint cascadeIndex = 0;
    if (viewDepth > gShadowData.cascadeSplits.x)
        cascadeIndex = 1;
    if (viewDepth > gShadowData.cascadeSplits.y)
        cascadeIndex = 2;
    if (viewDepth > gShadowData.cascadeSplits.z)
        cascadeIndex = 3;

    // Normal Bias (シャドウアクネ軽減)
    float biasScale = saturate(1.0f - NdotL);
    float3 biasedWorldPos = worldPos + normal * (gMaterial.shadowNormalBias * biasScale);

    float4 shadowCoord = mul(float4(biasedWorldPos, 1.0f), gShadowData.cascadeLightViewProj[cascadeIndex]);
    float3 projCoords = shadowCoord.xyz / shadowCoord.w;

    projCoords.x = projCoords.x * 0.5f + 0.5f;
    projCoords.y = -projCoords.y * 0.5f + 0.5f;

    float currentDepth = projCoords.z - gMaterial.shadowBias;

    // Frustum外のクリップ判定
    if (any(projCoords < 0.0f) || any(projCoords > 1.0f))
    {
        return 1.0f;
    }

    // Hardware PCF (SampleCmpLevelZero 1回で 2x2 bilinear 補間された結果を取得)
    float shadowVisibility = gShadowMapArray.SampleCmpLevelZero(
        gShadowSampler,
        float3(projCoords.xy, cascadeIndex),
        currentDepth
    );

    return lerp(minShadow, 1.0f, shadowVisibility);
}