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

SamplerComparisonState gShadowSampler : register(s1);
SamplerState gAnisoSampler : register(s3);

struct PixelInput
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

// 高速 Interleaved Gradient Noise (sin/cos不使用)
float InterleavedGradientNoise(float2 pixelPos)
{
    float3 magic = float3(0.06711056f, 0.00583715f, 52.9829189f);
    return frac(magic.z * frac(dot(pixelPos, magic.xy)));
}

// 高速 5乗計算 (pow命令の排除)
float Pow5(float x)
{
    float x2 = x * x;
    return x2 * x2 * x;
}

float CalculateFastShadowCSM(float3 worldPos, float3 normal, float viewDepth);

PixelShaderOutput main(PixelInput input, bool isFrontFace : SV_IsFrontFace)
{
    PixelShaderOutput output;

    // -------------------------------------------------------------------------
    // 1. LODディザリング (超軽量IGN)
    // -------------------------------------------------------------------------
    float dither = InterleavedGradientNoise(input.position.xy);
    clip(input.lodFade - dither);

    // -------------------------------------------------------------------------
    // 2. アルベド & アルファサンプリング
    // -------------------------------------------------------------------------
    float4 albedoAlpha = gAlbedoAlphaTex.Sample(gAnisoSampler, input.texcoord);
    clip(albedoAlpha.a - 0.05f); // ピクセル破棄時はここで即時離脱

    albedoAlpha.rgb *= input.instanceTint * gMaterial.colorTint * max(gMaterial.albedoMultiplier, 0.0f);

    // -------------------------------------------------------------------------
    // 3. 物理プロパティ & 濡れ (Wetness) 計算
    // -------------------------------------------------------------------------
    float ao = gMaterial.baseAO * input.color.a;

    // ① Porosity (濡れると吸光して暗く鮮やかになる)
    albedoAlpha.rgb = lerp(albedoAlpha.rgb, albedoAlpha.rgb * 0.45f, gEnvironmentData.wetness);
    
    // ② Roughness (濡れると水膜でツルツルになる)
    float roughness = lerp(gMaterial.baseRoughness, 0.05f, gEnvironmentData.wetness);

    // -------------------------------------------------------------------------
    // 4. 法線計算 (Bitangent動的算出 & ブランチレス裏面処理)
    // -------------------------------------------------------------------------
    float faceSign = isFrontFace ? 1.0f : -1.0f;
    float3 N = normalize(input.normal * faceSign);
    float3 T = normalize(input.tangent * faceSign);
    float3 B = cross(N, T); // 頂点補間帯域を削るためPS側でクロス積計算

    float3x3 TBN = float3x3(T, B, N);
    float3 tangentNormal = gNormalTex.Sample(gAnisoSampler, input.texcoord).xyz * 2.0f - 1.0f;
    float3 normal = normalize(mul(tangentNormal, TBN));

    // -------------------------------------------------------------------------
    // 5. ライティング基本ベクトル & 光の共通減衰
    // -------------------------------------------------------------------------
    float3 cameraDiff = gFrameData.cameraWorldPosition - input.worldPosition;
    float viewDepth = length(cameraDiff);
    float3 toEye = cameraDiff / max(viewDepth, 0.0001f);

    float3 lightDir = normalize(-gDirectionalLights[0].direction);
    float shadowFactor = CalculateFastShadowCSM(input.worldPosition, normal, viewDepth);
    
    // 共通光強度（シャドウ適用済み）を先打ち計算
    float3 attenuatedLight = (gDirectionalLights[0].color.rgb * gDirectionalLights[0].intensity) * shadowFactor;

    // --- Diffuse ---
    float wrap = gMaterial.diffuseWrap;
    float NdotL = saturate((dot(normal, lightDir) + wrap) / ((1.0f + wrap) * (1.0f + wrap)));
    float3 diffuse = albedoAlpha.rgb * attenuatedLight * NdotL;

    // --- 高速 Subsurface Scattering (SSS / 透過光) ---
    // 逆光(Backlight)成分の評価
    float backLight = saturate(dot(-normal, lightDir));
    float sssIntensity = Pow5(backLight) * (1.0f - gMaterial.baseThickness) * gMaterial.sssStrength;
    float3 transmission = (albedoAlpha.rgb * gMaterial.sssColor) * attenuatedLight * sssIntensity;

    // -------------------------------------------------------------------------
    // 6. Direct Specular (GGX + Pow5最適化)
    // -------------------------------------------------------------------------
    float3 halfVector = normalize(lightDir + toEye);
    float NdotH = saturate(dot(normal, halfVector));
    float NdotV = saturate(dot(normal, toEye));
    
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    float denom = (NdotH * NdotH * (alpha2 - 1.0f) + 1.0f);
    float D = alpha2 / (3.14159265f * denom * denom + 0.00001f);

    // 水のF0 = 0.02, 葉のF0 = 0.04
    float3 F0 = lerp(0.04f.xxx, 0.02f.xxx, gEnvironmentData.wetness);
    float3 F = F0 + (1.0f.xxx - F0) * Pow5(1.0f - saturate(dot(halfVector, toEye)));

    float gustMask = input.color.r;
    float3 directSpecular = (D * F) * attenuatedLight * NdotL * (1.0f + gustMask * 2.0f);

    // -------------------------------------------------------------------------
    // 7. IBL Specular & Ambient (CubeMap不要の数式近似)
    // -------------------------------------------------------------------------
    float3 reflectDir = reflect(-toEye, normal);

    // 反射ベクトルのY成分(高さ)から空色と地面色を直接補間
    float skyWeight = saturate(reflectDir.y * 0.5f + 0.5f);
    float3 envSkyColor = lerp(gEnvironmentData.groundColor.rgb, gEnvironmentData.skyColor.rgb, skyWeight);
    envSkyColor *= (1.0f - roughness * 0.5f); // ラフネスに応じた減衰

    // フレネル (環境光用)
    float3 F_env = F0 + (max(1.0f.xxx - roughness, F0) - F0) * Pow5(1.0f - NdotV);
    float3 ambientSpecular = envSkyColor * F_env * ao;

    // ディフューズ環境光 (法線Y成分による半球ライティング)
    float skyLight = saturate(normal.y * 0.5f + 0.5f);
    float3 ambientDiffuse = albedoAlpha.rgb * lerp(gEnvironmentData.groundColor.rgb, gEnvironmentData.skyColor.rgb, skyLight) * ao;

    // -------------------------------------------------------------------------
    // 最終カラー合成 & Alpha-to-Coverage
    // -------------------------------------------------------------------------
    float3 finalColor = diffuse + transmission + directSpecular + ambientDiffuse + ambientSpecular;

    float outAlpha = (albedoAlpha.a - gMaterial.alphaCutoff) / max(fwidth(albedoAlpha.a), 0.0001f) + 0.5f;
    
    output.color = float4(finalColor, saturate(outAlpha));
    output.normal = float4(normal, 1.0f);
    output.material = float4(0.0f, roughness, 0.0f, 1.0f);
    output.velocity = float2(0.0f, 0.0f);

    return output;
}

// -----------------------------------------------------------------------------
// 高速 CSM フェッチ (ブランチレス化)
// -----------------------------------------------------------------------------
float CalculateFastShadowCSM(float3 worldPos, float3 normal, float viewDepth)
{
    float3 lightDir = normalize(-gDirectionalLights[0].direction);
    float NdotL = dot(normal, lightDir);
    float minShadow = 1.0f - saturate(gMaterial.shadowDensity);

    // 光の裏側（セルフシャドウ領域）はフェッチを完全スキップ
    if (NdotL <= 0.0f)
        return minShadow;

    // ブランチ(if文)無しのカスケード選択
    float4 cascadeSplits = gShadowData.cascadeSplits;
    uint cascadeIndex = (uint) dot(step(cascadeSplits.xyz, viewDepth.xxx), float3(1.0f, 1.0f, 1.0f));

    // Normal Bias
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