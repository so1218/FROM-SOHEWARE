#include "Common/Object3D.hlsli"
#include "Common/ShaderConstants.hlsli"
#include "Common/MathUtils.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);

cbuffer DirectionalLights : register(b1)
{
    DirectionalLight gDirectionalLights[MAX_DIRECTIONAL_LIGHTS];
};

ConstantBuffer<GrassMaterialData> gMaterial : register(b5);
ConstantBuffer<ShadowData> gShadowData : register(b8);

Texture2DArray<float> gShadowMapArray : register(t2);
SamplerComparisonState gShadowSampler : register(s1);

struct GrassPSInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 worldPosition : WORLD_POSITION;
    float4 color : COLOR;
};

float CalculateFastShadowCSM(float3 worldPos, float3 normal, float viewDepth);

PixelShaderOutput main(GrassPSInput input)
{
    PixelShaderOutput output;
    
    // カメラからの距離計算
    float3 cameraDiff = gFrameData.cameraWorldPosition - input.worldPosition;
    float viewDepth = length(cameraDiff);

    // カメラ近接フェード率の算出とディザリングクリップ
    float proximityFade = saturate((viewDepth - gMaterial.nearFadeMinDist) / max(gMaterial.nearFadeMaxDist - gMaterial.nearFadeMinDist, kEpsilon));
    float dither = InterleavedGradientNoise(input.position.xy);
    clip(proximityFade - dither);

    float t = input.texcoord.y;
    float gustMask = input.color.a;

    // ベースカラーの取得と Wetnessによる暗化
    float3 baseColor = input.color.rgb;
    float wetDarkening = lerp(1.0f, 0.6f, gMaterial.wetness);
    baseColor *= wetDarkening;

    float3 toEye = normalize(gFrameData.cameraWorldPosition - input.worldPosition);
    float3 lightDir = normalize(-gDirectionalLights[0].direction);
    
    // 面全体で柔らかく光を受けるように上方向へ法線をブレンド
    float3 bladeNormal = normalize(input.normal);
    float3 normal = normalize(lerp(bladeNormal, float3(0.0f, 1.0f, 0.0f), gMaterial.grassNormalBlend));
    
    float shadowFactor = CalculateFastShadowCSM(input.worldPosition, normal, viewDepth);

    // -------------------------------------------------------------------------
    // Foliage Shading (Diffuse & SSS)
    // -------------------------------------------------------------------------
    // 陰への光の回り込み
    float wrap = 0.5f;
    float NdotL = saturate((dot(normal, lightDir) + wrap) / ((1.0f + wrap) * (1.0f + wrap)));
    float3 diffuse = baseColor * gDirectionalLights[0].color.rgb * gDirectionalLights[0].intensity * NdotL;

    // View-dependent SSS (逆光時の葉の透過)
    float sssDot = saturate(dot(-lightDir, toEye + bladeNormal * 0.2f));
    float sssFade = pow(sssDot, 2.5f) * t;
    float3 translucency = gMaterial.sssColor * sssFade * gMaterial.sssStrength * gDirectionalLights[0].color.rgb;

    diffuse *= shadowFactor;
    translucency *= shadowFactor;

    // 環境光と疑似AO (根本を暗くして接地感を出す)
    float ao = lerp(gMaterial.grassRootAO, 1.0f, t);
    float skyLight = saturate(normal.y * 0.5f + 0.5f);
    float3 ambient = baseColor * (0.15f + skyLight * 0.2f) * ao;

    float3 finalColor = diffuse + translucency + ambient;

    // -------------------------------------------------------------------------
    // Specular
    // -------------------------------------------------------------------------
    float3 H = normalize(lightDir + toEye);
    float3 tangent = normalize(input.tangent);
    
    float TdotH = dot(tangent, H);
    float sinTH = sqrt(1.0f - saturate(TdotH * TdotH));
    
    // 濡れるとスペキュラが鋭く強くなる
    float shininess = lerp(gMaterial.specularShininess, 150.0f, gMaterial.wetness);
    float specIntensity = pow(sinTH, shininess) * gMaterial.specularStrength * lerp(1.0f, 2.5f, gMaterial.wetness);
    specIntensity *= saturate(dot(normal, lightDir)) * shadowFactor;
    
    // 突風時の面の揃いによるハイライト
    specIntensity *= 1.0f + (gustMask * gMaterial.windHighlightStrength * t);

    finalColor += gDirectionalLights[0].color.rgb * specIntensity * gDirectionalLights[0].intensity;

    // 出力
    output.color = float4(finalColor, 1.0f);
    output.normal = float4(normal, 1.0f);
    
    // G-Buffer : 濡れ表現でRoughnessを下げる
    output.material = float4(0.0f, 0.8f - (gMaterial.wetness * 0.6f), 0.0f, 1.0f);

    return output;
}

// -----------------------------------------------------------------------------
// Foliage向け 軽量CSMフェッチ
// -----------------------------------------------------------------------------
// 膨大なピクセル面積を占める草描画の帯域幅を節約するため、1-Tap PCFで済ませる
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