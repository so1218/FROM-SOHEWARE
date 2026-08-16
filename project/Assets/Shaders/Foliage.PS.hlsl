#include "Object3D.hlsli"
#include "ShaderConstants.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);
cbuffer DirectionalLights : register(b1)
{
    DirectionalLight gDirectionalLights[MAX_DIRECTIONAL_LIGHTS];
};
ConstantBuffer<GlobalEnvironmentData> gEnvironmentData : register(b4);
ConstantBuffer<FoliageMaterialData> gMaterial : register(b5);
ConstantBuffer<ShadowData> gShadowData : register(b8);


// ★ バインドレス・テクスチャ配列 (マテリアルIDでアクセス)
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
    
    // color.r = 風の揺れやすさ (0.0=根元, 1.0=先端) を兼ねる
    float4 color : COLOR0;
    float3 instanceTint : COLOR1;
    float2 velocity : TEXCOORD1;
};

// 高速 5乗計算
float Pow5(float x)
{
    float x2 = x * x;
    return x2 * x2 * x;
}

// カスケードシャドウ (Leaf PSと同一の高速版を想定)
float CalculateFastShadowCSM(float3 worldPos, float3 normal, float viewDepth);

PixelShaderOutput main(FoliagePSInput input, bool isFrontFace : SV_IsFrontFace)
{
    PixelShaderOutput output;

    // ★ 配列ではなく単一のテクスチャからサンプリング
    float4 albedoAlpha = gAlbedoAlphaTex.Sample(gAnisoSampler, input.texcoord);
    
    // 早期ピクセル破棄
    clip(albedoAlpha.a - 0.05f);

    // インスタンスごとの色ムラ（花畑の自然なばらつき）
    albedoAlpha.rgb *= gMaterial.baseColor * input.instanceTint;

    // -------------------------------------------------------------------------
    // RDR2手法: 根元のグラデーションブレンド (Mud & Wetness)
    // -------------------------------------------------------------------------
    float rootMask = saturate(1.0f - input.color.r);
    rootMask = pow(rootMask, 2.0f);

    // ① 濡れによる吸光 (Porosity)
    albedoAlpha.rgb = lerp(albedoAlpha.rgb, albedoAlpha.rgb * 0.45f, gEnvironmentData.wetness);

    // ③ ラフネス
    float currentRoughness = lerp(gMaterial.roughness, 0.1f, gEnvironmentData.wetness * (1.0f - rootMask));

    // -------------------------------------------------------------------------
    // 法線計算 & RDR2手法: 接地法線ブレンド
    // -------------------------------------------------------------------------
    float faceSign = isFrontFace ? 1.0f : -1.0f;
    float3 N = normalize(input.normal * faceSign);
    float3 T = normalize(input.tangent * faceSign);
    float3 B = cross(N, T);
    float3x3 TBN = float3x3(T, B, N);
    
    // ★ 単一のノーマルマップからサンプリング
    float3 tangentNormal = gNormalTex.Sample(gAnisoSampler, input.texcoord).xyz * 2.0f - 1.0f;
    float3 worldNormal = normalize(mul(tangentNormal, TBN));

    // 根元の法線を上に向ける
    worldNormal = normalize(lerp(worldNormal, float3(0.0f, 1.0f, 0.0f), rootMask * 0.5f));

    // -------------------------------------------------------------------------
    // ライティング基本 & 高速CSM
    // -------------------------------------------------------------------------
    float3 cameraDiff = gFrameData.cameraWorldPosition - input.worldPosition;
    float viewDepth = length(cameraDiff);
    float3 toEye = cameraDiff / max(viewDepth, 0.0001f);
    float3 lightDir = normalize(-gDirectionalLights[0].direction);
    
    // ★ 引数を3つに修正
    float shadowFactor = CalculateFastShadowCSM(input.worldPosition, worldNormal, viewDepth);
    float3 attenuatedLight = (gDirectionalLights[0].color.rgb * gDirectionalLights[0].intensity) * shadowFactor;

    // --- Diffuse ---
    float wrap = 0.3f;
    float NdotL = saturate((dot(worldNormal, lightDir) + wrap) / ((1.0f + wrap) * (1.0f + wrap)));
    float3 diffuse = albedoAlpha.rgb * attenuatedLight * NdotL;

    // -------------------------------------------------------------------------
    // GoT手法: Subsurface Scattering (透過光)
    // -------------------------------------------------------------------------
    float backLight = saturate(dot(-worldNormal, lightDir));
    float sssIntensity = Pow5(backLight) * gMaterial.sssStrength;
    
    float3 sssColor = albedoAlpha.rgb * 1.5f;
    float3 transmission = (albedoAlpha.rgb * 1.5f) * attenuatedLight * sssIntensity;

    // -------------------------------------------------------------------------
    // GoT手法: Wind Gust Mask を用いた Direct Specular
    // -------------------------------------------------------------------------
    float3 halfVector = normalize(lightDir + toEye);
    float NdotH = saturate(dot(worldNormal, halfVector));
    float NdotV = saturate(dot(worldNormal, toEye));
    
    float alpha = currentRoughness * currentRoughness;
    float alpha2 = alpha * alpha;
    float denom = (NdotH * NdotH * (alpha2 - 1.0f) + 1.0f);
    float D = alpha2 / (3.14159265f * denom * denom + 0.00001f);

    float3 F0 = lerp(0.04f.xxx, 0.02f.xxx, gEnvironmentData.wetness);
    float3 F = F0 + (1.0f.xxx - F0) * Pow5(1.0f - saturate(dot(halfVector, toEye)));

    float gustMask = input.color.r;
    float3 directSpecular = (D * F) * attenuatedLight * NdotL * (1.0f + gustMask * 1.5f);

    // -------------------------------------------------------------------------
    // IBL & 環境光
    // -------------------------------------------------------------------------
    float3 reflectDir = reflect(-toEye, worldNormal);
    float skyWeight = saturate(reflectDir.y * 0.5f + 0.5f);
    float3 envSkyColor = lerp(gEnvironmentData.groundColor.rgb, gEnvironmentData.skyColor.rgb, skyWeight);
    envSkyColor *= (1.0f - currentRoughness * 0.5f);

    float3 F_env = F0 + (max(1.0f.xxx - currentRoughness, F0) - F0) * Pow5(1.0f - NdotV);
    float3 ambientSpecular = envSkyColor * F_env;

    float skyLight = saturate(worldNormal.y * 0.5f + 0.5f);
    float3 ambientDiffuse = albedoAlpha.rgb * lerp(gEnvironmentData.groundColor.rgb, gEnvironmentData.skyColor.rgb, skyLight);

    float ao = lerp(0.2f, 1.0f, input.color.r);

    // -------------------------------------------------------------------------
    // 最終出力
    // -------------------------------------------------------------------------
    float3 finalColor = diffuse + transmission + directSpecular + (ambientDiffuse + ambientSpecular) * ao;

    float outAlpha = (albedoAlpha.a - gMaterial.alphaCutoff) / max(fwidth(albedoAlpha.a), 0.0001f) + 0.5f;
    
    output.color = float4(finalColor, saturate(outAlpha));
    output.normal = float4(worldNormal, 1.0f);
    output.material = float4(0.0f, currentRoughness, 0.0f, 1.0f);
    output.velocity = input.velocity;

    return output;
}

// -----------------------------------------------------------------------------
// 高速 CSM フェッチ (エラー修正済)
// -----------------------------------------------------------------------------
// ★ 宣言通りの引数3つで定義
float CalculateFastShadowCSM(float3 worldPos, float3 normal, float viewDepth)
{
    float3 lightDir = normalize(-gDirectionalLights[0].direction);
    float NdotL = dot(normal, lightDir);
    // ★ グローバルの gMaterial を直接参照
    float minShadow = 1.0f - saturate(gMaterial.shadowDensity);

    // 光の裏側（セルフシャドウ領域）はフェッチを完全スキップ
    if (NdotL <= 0.0f)
        return minShadow;

    float4 cascadeSplits = gShadowData.cascadeSplits;
    uint cascadeIndex = (uint) dot(step(cascadeSplits.xyz, viewDepth.xxx), float3(1.0f, 1.0f, 1.0f));

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