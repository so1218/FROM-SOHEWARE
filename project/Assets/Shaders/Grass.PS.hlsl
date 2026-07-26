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

// シャドウ強度を計算
float CalculateFastShadowCSM(float3 worldPos, float3 normal, float viewDepth);

PixelShaderOutput main(PixelInput input)
{
    PixelShaderOutput output;
    
    float t = input.texcoord.y; // 0.0(根本) ~ 1.0(先端)
    float gustMask = input.color.a;

    // --- 1. ベースカラーグラデーション ---
    float3 grassColor = lerp(gMaterial.rootColor, gMaterial.tipColor, t);
    float3 baseColor = input.color.rgb * grassColor;

    float3 toEye = normalize(gFrameData.cameraWorldPosition - input.worldPosition);
    float3 lightDir = normalize(-gDirectionalLights[0].direction);
    
    // AAAの重要アプローチ：個々の法線を「空(0,1,0)」へブレンドしてチラツキを防止！
    float3 bladeNormal = normalize(input.normal);
    float3 globalUpNormal = float3(0.0f, 1.0f, 0.0f);
    float3 normal = normalize(lerp(bladeNormal, globalUpNormal, gMaterial.grassNormalBlend));

    // --- 2. 影の計算 ---
    float viewDepth = distance(gFrameData.cameraWorldPosition, input.worldPosition);
    float shadowFactor = CalculateFastShadowCSM(input.worldPosition, normal, viewDepth);
    float flashIntensity = saturate(gFrameData.lightningFlashIntensity);
    shadowFactor = lerp(shadowFactor, 1.0f, flashIntensity);

    // --- 3. Foliage Light (Wrap Diffuse & SSS) ---
    // ① Wrap Diffuse (光の柔らかい回り込み)
    float wrap = 0.5f;
    float NdotL = saturate((dot(normal, lightDir) + wrap) / ((1.0f + wrap) * (1.0f + wrap)));
    float3 diffuse = baseColor * gDirectionalLights[0].color.rgb * gDirectionalLights[0].intensity * NdotL;

    // ② Subsurface Scattering (透過光)
    // 逆光時に葉を透過する美しい光 (先端 t ほど強く透ける)
    float sssDot = saturate(dot(-lightDir, toEye + bladeNormal * 0.2f));
    float sssFade = pow(sssDot, 2.5f) * t;
    float3 translucency = gMaterial.sssColor * sssFade * gMaterial.sssStrength * gDirectionalLights[0].color.rgb;

    diffuse *= shadowFactor;
    translucency *= shadowFactor;

    // ③ Ambient & 根本のAO
    float ao = lerp(gMaterial.grassRootAO, 1.0f, t);
    float skyLight = saturate(normal.y * 0.5f + 0.5f);
    float3 ambient = baseColor * (0.15f + skyLight * 0.2f + gFrameData.lightningFlashColor * flashIntensity) * ao;

    float3 finalColor = diffuse + translucency + ambient;

    // --- 4. 暴れない上品なスペキュラ (Tsushima Satin Specular) ---
    float3 H = normalize(lightDir + toEye);
    
    // Kajiya-Kay 風の縦方向異方性ハイライト
    float3 tangent = normalize(input.tangent);
    float TdotH = dot(tangent, H);
    float sinTH = sqrt(1.0f - saturate(TdotH * TdotH));
    
    float shininess = lerp(gMaterial.specularShininess, 150.0f, gMaterial.wetness);
    float specIntensity = pow(sinTH, shininess) * gMaterial.specularStrength;
    
    // 光の表面だけで反応するようシャドウとNdotLでマスク
    specIntensity *= saturate(dot(normal, lightDir)) * shadowFactor;

    // ★ 風による「サテン光沢（シルバーライニング）」の自然な補正
    // 直接色を乗算・加算して発光させるのではなく、風が吹く場所のスペキュラ幅をわずかに引き締める
    float windBoost = 1.0f + (gustMask * gMaterial.windHighlightStrength * t);
    specIntensity *= windBoost;

    float3 specular = gDirectionalLights[0].color.rgb * specIntensity * gDirectionalLights[0].intensity;
    finalColor += specular;

    // 出力
    output.color = float4(finalColor, 1.0f);
    output.normal = float4(normal, 1.0f);
    output.material = float4(0.0f, 0.8f - (gMaterial.wetness * 0.6f), 0.0f, 1.0f);
    
    float2 ndcCurrent = input.currentClipPos.xy / input.currentClipPos.w;
    float2 ndcPrev = input.prevClipPos.xy / input.prevClipPos.w;
    output.velocity = (ndcCurrent - ndcPrev) * float2(0.5f, -0.5f);

    return output;
}

// 草専用の超軽量シャドウ計算 (1タップ)
float CalculateFastShadowCSM(float3 worldPos, float3 normal, float viewDepth)
{
    float3 lightDir = normalize(-gDirectionalLights[0].direction);
    float NdotL = dot(normal, lightDir);
    float minShadow = 1.0f - saturate(gMaterial.shadowDensity);

    // 光の裏側なら即座に暗くする（サンプリングすらしない）
    if (NdotL <= 0.0f)
        return minShadow;

    // カスケード判定
    uint cascadeIndex = 0;
    if (viewDepth > gShadowData.cascadeSplits.x)
        cascadeIndex = 1;
    if (viewDepth > gShadowData.cascadeSplits.y)
        cascadeIndex = 2;
    if (viewDepth > gShadowData.cascadeSplits.z)
        cascadeIndex = 3;

    // シャドウバイアス
    float biasScale = saturate(1.0f - NdotL);
    float worldNormalBias = gMaterial.shadowNormalBias * biasScale;
    float3 biasedWorldPos = worldPos + normal * worldNormalBias;

    float4 shadowCoord = mul(float4(biasedWorldPos, 1.0f), gShadowData.cascadeLightViewProj[cascadeIndex]);
    float3 projCoords = shadowCoord.xyz / shadowCoord.w;

    projCoords.x = projCoords.x * 0.5f + 0.5f;
    projCoords.y = -projCoords.y * 0.5f + 0.5f;

    float currentDepth = projCoords.z - gMaterial.shadowBias;

    if (projCoords.z < 0.0f || projCoords.z > 1.0f ||
        projCoords.x < 0.0f || projCoords.x > 1.0f ||
        projCoords.y < 0.0f || projCoords.y > 1.0f)
    {
        return 1.0f;
    }

    // ハードウェアPCF（SampleCmpLevelZero 1回で2x2の補間シャドウが得られる）
    float shadowVisibility = gShadowMapArray.SampleCmpLevelZero(
        gShadowSampler,
        float3(projCoords.xy, cascadeIndex),
        currentDepth
    );

    return lerp(minShadow, 1.0f, shadowVisibility);
}