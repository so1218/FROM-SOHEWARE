#include "Object3D.hlsli"
#include "ShaderConstants.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);

cbuffer DirectionalLights : register(b1)
{
    DirectionalLight gDirectionalLights[MAX_DIRECTIONAL_LIGHTS];
};

ConstantBuffer<GrassMaterialData> gMaterial : register(b5);
ConstantBuffer<ShadowData> gShadowData : register(b8);

Texture2DArray<float> gShadowMapArray : register(t2);
SamplerComparisonState gShadowSampler : register(s1);

struct PixelInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 worldPosition : WORLD_POSITION;
    float4 color : COLOR;
    float4 currentClipPos : POSITION1;
    float4 prevClipPos : POSITION2;
};

float CalculateFastShadowCSM(float3 worldPos, float3 normal, float viewDepth);

PixelShaderOutput main(PixelInput input)
{
    PixelShaderOutput output;
    
    float t = input.texcoord.y;
    float gustMask = input.color.a;

    // 頂点カラーに焼き付けたベースカラーとの合成
    float3 grassColor = lerp(gMaterial.rootColor, gMaterial.tipColor, t);
    float3 baseColor = input.color.rgb * grassColor;

    float3 toEye = normalize(gFrameData.cameraWorldPosition - input.worldPosition);
    float3 lightDir = normalize(-gDirectionalLights[0].direction);
    
    // Foliage特有の高周波な法線によるピクセル単位のチラツキを抑えるため、
    // 上方向(0,1,0)へ法線をブレンドし、面全体で柔らかく光を受けるように補正
    float3 bladeNormal = normalize(input.normal);
    float3 normal = normalize(lerp(bladeNormal, float3(0.0f, 1.0f, 0.0f), gMaterial.grassNormalBlend));

    float viewDepth = distance(gFrameData.cameraWorldPosition, input.worldPosition);
    float shadowFactor = CalculateFastShadowCSM(input.worldPosition, normal, viewDepth);

    // -------------------------------------------------------------------------
    // Foliage Shading (Diffuse & SSS)
    // -------------------------------------------------------------------------
    // 葉の円柱的な構造を近似し、陰への光の回り込みを表現
    float wrap = 0.5f;
    float NdotL = saturate((dot(normal, lightDir) + wrap) / ((1.0f + wrap) * (1.0f + wrap)));
    float3 diffuse = baseColor * gDirectionalLights[0].color.rgb * gDirectionalLights[0].intensity * NdotL;

    // View-dependent SSS: 逆光時に葉を透過する光の近似
    // 根本(t=0)は厚みがあるとして減衰させ、先端(t=1)ほど強く透過
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
    // 接線ベースの縦方向ハイライト
    float3 H = normalize(lightDir + toEye);
    float3 tangent = normalize(input.tangent);
    
    float TdotH = dot(tangent, H);
    float sinTH = sqrt(1.0f - saturate(TdotH * TdotH));
    
    float shininess = lerp(gMaterial.specularShininess, 150.0f, gMaterial.wetness);
    float specIntensity = pow(sinTH, shininess) * gMaterial.specularStrength;
    specIntensity *= saturate(dot(normal, lightDir)) * shadowFactor; // 陰部分のハイライト遮蔽

    // Wind Specular Modulation
    // 突風マスクを利用し、風が強く当たる領域のスペキュラ輝度を引き上げる
    // 草が風になびいた瞬間に面が揃って白く光る現象を低負荷で近似
    specIntensity *= 1.0f + (gustMask * gMaterial.windHighlightStrength * t);

    finalColor += gDirectionalLights[0].color.rgb * specIntensity * gDirectionalLights[0].intensity;

    // 出力
    output.color = float4(finalColor, 1.0f);
    output.normal = float4(normal, 1.0f);
    
    // G-Buffer : 濡れ表現でRoughnessを下げる
    output.material = float4(0.0f, 0.8f - (gMaterial.wetness * 0.6f), 0.0f, 1.0f);
    
    // Motion Vector (TAA / Motion Blur用)
    float2 ndcCurrent = input.currentClipPos.xy / input.currentClipPos.w;
    float2 ndcPrev = input.prevClipPos.xy / input.prevClipPos.w;
    output.velocity = (ndcCurrent - ndcPrev) * float2(0.5f, -0.5f);

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