#include "Object3D.hlsli"
#include "ShaderConstants.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);
cbuffer DirectionalLights : register(b1)
{
    DirectionalLight gDirectionalLights[MAX_DIRECTIONAL_LIGHTS];
};
ConstantBuffer<LeafMaterialData> gMaterial : register(b5);
ConstantBuffer<ShadowData> gShadowData : register(b8);
StructuredBuffer<TreeInstanceData> gInstanceData : register(t10);

// 風マップとテクスチャ群
Texture2DArray<float> gShadowMapArray : register(t2);
Texture2D<float4> gAlbedoAlphaTex : register(t12);
Texture2D<float3> gNormalTex : register(t13);
Texture2D<float4> gMetallicRoughnessTex : register(t14);

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
    
    // LODディザリング
    float dither = frac(sin(dot(input.position.xy, float2(12.9898f, 78.233f))) * 43758.5453f);
    clip(input.lodFade - dither);

    float4 albedoAlpha = gAlbedoAlphaTex.Sample(gAnisoSampler, input.texcoord);
    clip(albedoAlpha.a - 0.05f);
    albedoAlpha.rgb *= input.instanceTint;

    float4 mrTex = gMetallicRoughnessTex.Sample(gAnisoSampler, input.texcoord);
    float roughness = mrTex.g * gMaterial.roughnessScale;
    
    // ==========================================
    // 【修正】AOの計算
    // テクスチャにAOがある場合はそれを使用し、
    // VSで計算した「擬似的な頂点AO (input.color.a)」を掛け合わせる
    // ==========================================
    float texAO = (mrTex.r > 0.001f) ? mrTex.r : 1.0f;
    float pseudoAO = input.color.a; // VSから受け取ったハックAO
    float ao = texAO * gMaterial.baseAO * pseudoAO;

    float thickness = gMaterial.baseThickness;

    // 両面描画の法線対応
    float3 N = normalize(input.normal);
    float3 T = normalize(input.tangent);
    float3 B = normalize(input.bitangent);
    
    if (!isFrontFace)
    {
        N = -N;
        T = -T;
        B = -B;
        N = normalize(lerp(N, input.normal, gMaterial.backfaceFlatten));
    }
    
    float3x3 TBN = float3x3(T, B, N);
    float3 tangentNormal = gNormalTex.Sample(gAnisoSampler, input.texcoord).xyz * 2.0f - 1.0f;
    float3 normal = normalize(mul(tangentNormal, TBN));

    // ライティング計算
    float3 toEye = normalize(gFrameData.cameraWorldPosition - input.worldPosition);
    float3 lightDir = normalize(-gDirectionalLights[0].direction);
    float3 lightColor = gDirectionalLights[0].color.rgb * gDirectionalLights[0].intensity;
    
    float viewDepth = distance(gFrameData.cameraWorldPosition, input.worldPosition);
    float shadowFactor = CalculateFastShadowCSM(input.worldPosition, normal, viewDepth);

    // Diffuse (Wrap)
    float wrap = gMaterial.diffuseWrap;
    float NdotL = saturate((dot(normal, lightDir) + wrap) / ((1.0f + wrap) * (1.0f + wrap)));
    float3 diffuse = albedoAlpha.rgb * lightColor * NdotL;

    // Transmission (透過光)
    float3 h = normalize(lightDir + normal * gMaterial.transmissionDistortion);
    float VdotH = saturate(dot(toEye, -h));
    float transmissionProfile = pow(VdotH, gMaterial.transmissionPower);
    float sssIntensity = transmissionProfile * (1.0f - thickness) * gMaterial.sssStrength;
    float3 transmissionColor = albedoAlpha.rgb * gMaterial.sssColor;
    float3 transmission = transmissionColor * lightColor * sssIntensity;

    diffuse *= shadowFactor;
    transmission *= shadowFactor;

    // Ambient
    float skyLight = saturate(normal.y * 0.5f + 0.5f);
    float3 ambient = albedoAlpha.rgb * (0.1f + skyLight * 0.25f) * ao;

    // Specular (PBR)
    float3 halfVector = normalize(lightDir + toEye);
    float NdotH = saturate(dot(normal, halfVector));
    float distanceRoughness = saturate(roughness + (viewDepth * 0.002f));
    
    float alpha = distanceRoughness * distanceRoughness;
    float alpha2 = alpha * alpha;
    float denom = (NdotH * NdotH * (alpha2 - 1.0f) + 1.0f);
    float d = alpha2 / (3.14159f * denom * denom);
    
    float3 specular = d * lightColor * shadowFactor * 0.1f;
    
    // ==========================================
    // 【修正】Gust(突風)によるスペキュラの強調
    // ==========================================
    float gustMask = input.color.r; // VSから受け取ったGustMask
    specular *= 1.0f + (gustMask * 2.0f); // 風が吹くと葉が裏返り、光沢が強くなる表現

    float3 finalColor = diffuse + transmission + ambient + specular;

    // Alpha-to-Coverage (A2C)
    float outAlpha = (albedoAlpha.a - gMaterial.alphaCutoff) / max(fwidth(albedoAlpha.a), 0.0001f) + 0.5f;
    
    output.color = float4(finalColor, saturate(outAlpha));
    output.normal = float4(normal, 1.0f);
    output.material = float4(distanceRoughness, 0.0f, 0.0f, 1.0f);

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