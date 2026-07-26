#include "ShaderConstants.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);

// 入力テクスチャ
Texture2D<float4> gHitResultTexture : register(t0);
Texture2D<float4> gSceneTexture : register(t1);
Texture2D<float4> gNormalTexture : register(t2);
Texture2D<float> gDepthTexture : register(t3);
Texture2D<float4> gMaterialTexture : register(t4);

// 出力テクスチャ
RWTexture2D<float4> gOutReflection : register(u0);
SamplerState gLinearSampler : register(s0);

// --- ユーティリティ関数 ---

float3 GetViewPos(float2 uv, float depth)
{
    float x = uv.x * 2.0f - 1.0f;
    float y = (1.0f - uv.y) * 2.0f - 1.0f;
    float4 clipPos = float4(x, y, depth, 1.0f);
    float4 viewPos = mul(clipPos, gFrameData.invProjMatrix);
    return viewPos.xyz / viewPos.w;
}

// ============================================================================
// BRDF LUTの解析的近似関数 (Karis, 2014)
// テクスチャを使わずに、数式で (Scale, Bias) を高速に近似計算
// ============================================================================
float2 EnvBRDFApprox(float roughness, float NdotV)
{
    const float4 c0 = float4(-1.0f, -0.0275f, -0.572f, 0.022f);
    const float4 c1 = float4(1.0f, 0.0425f, 1.04f, -0.04f);
    float4 r = roughness * c0 + c1;
    float a004 = min(r.x * r.x, exp2(-9.28f * NdotV)) * r.x + r.y;
    float2 AB = float2(-1.04f, 1.04f) * a004 + r.zw;
    return AB; // x = Scale (F0への乗数), y = Bias (加算値)
}

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint width, height;
    gOutReflection.GetDimensions(width, height);
    if (DTid.x >= width || DTid.y >= height)
        return;

    float2 uv = (float2(DTid.xy) + 0.5f) / float2(width, height);

    float4 hitData = gHitResultTexture.Load(int3(DTid.xy, 0));
    float2 hitUV = hitData.xy;
    float hitAlpha = hitData.z;

    if (hitAlpha <= 0.0f)
    {
        gOutReflection[DTid.xy] = float4(0, 0, 0, 0);
        return;
    }

    float depth = gDepthTexture.Load(int3(DTid.xy, 0));
    float4 material = gMaterialTexture.Load(int3(DTid.xy, 0));
    float metalness = material.r;
    float roughness = material.g;
    float3 worldNormal = gNormalTexture.Load(int3(DTid.xy, 0)).xyz;
    
    float3 viewPos = GetViewPos(uv, depth);
    float3 N = normalize(mul(worldNormal, (float3x3) gFrameData.viewMatrix));
    float3 V = normalize(-viewPos);
    
    float NdotV = max(dot(N, V), 0.001f);

    float3 albedo = gSceneTexture.Load(int3(DTid.xy, 0)).rgb;
    float3 f0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metalness);

    float maxSceneMip = 5.0f;
    float mipLevel = roughness * maxSceneMip;
    float3 hitColor = gSceneTexture.SampleLevel(gLinearSampler, hitUV, mipLevel).rgb;

    // ========================================================================
    // TODO: 【品質向上のための技術的負債】
    // 現在は処理を簡略化するため、BRDF LUTテクスチャのサンプリングを
    // EnvBRDFApprox() 関数による数式近似で代用しています。
    // 
    // 後日、エンジン初期化時にCSで事前計算した 256x256 の BRDF LUT テクスチャを
    // 生成・バインドする仕組みを構築し、以下の処理を置き換えてください。
    // 
    // [置き換え予定のコード]
    // float2 brdfLUT = gBrdfLutTexture.SampleLevel(gLinearSampler, float2(NdotV, roughness), 0).rg;
    // ========================================================================
    
    // 近似関数を使用してScaleとBiasを取得
    float2 brdfLUT = EnvBRDFApprox(roughness, NdotV);
    
    float3 specularReflectance = f0 * brdfLUT.x + brdfLUT.y;

    // 最終反射カラーの計算
    float3 finalReflection = hitColor * specularReflectance;

    // ファイアフライ（高輝度ノイズ）除去
    finalReflection = min(finalReflection, float3(10.0f, 10.0f, 10.0f));

    gOutReflection[DTid.xy] = float4(finalReflection * hitAlpha, hitAlpha);
}