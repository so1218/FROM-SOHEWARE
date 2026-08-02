#include "ShaderConstants.hlsli"

// Distribution (ハイライトの形状と強さ)
float D_GGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0f);
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
    denom = PI * denom * denom;

    return nom / max(denom, kExtinctionEpsilon);
}

// Geometry (表面の微細な凹凸による遮蔽)
float G_SchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0f);
    float k = (r * r) / 8.0f;

    float nom = NdotV;
    float denom = NdotV * (1.0f - k) + k;

    return nom / max(denom, kExtinctionEpsilon);
}

float G_Smith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0f);
    float NdotL = max(dot(N, L), 0.0f);
    float ggx1 = G_SchlickGGX(NdotV, roughness);
    float ggx2 = G_SchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

// 粗さを考慮したFresnel
float3 F_SchlickRoughness(float cosTheta, float3 F0, float roughness)
{
    // 粗い材質ほど、最大反射率（F90）を下げる
    float maxReflectance = 1.0f - roughness;
    float3 F90 = max(float3(maxReflectance, maxReflectance, maxReflectance), F0);
    
    return F0 + (F90 - F0) * pow(clamp(1.0f - cosTheta, 0.0f, 1.0f), 5.0f);
}

// 単一のライトに対するPBR計算（Cook-Torrance BRDF）
float3 CalculatePBR(
    float3 albedo,
    float3 N,
    float3 V,
    float3 L,
    float3 lightColor,
    float lightIntensity,
    float roughness,
    float metalness)
{
    float3 H = normalize(V + L); // ハーフベクトル

    // PBRパラメータの準備
    float3 F0 = float3(0.04f, 0.04f, 0.04f);
    F0 = lerp(F0, albedo, metalness);

    // BRDF項の計算
    float NDF = D_GGX(N, H, roughness);
    float G = G_Smith(N, V, L, roughness);
    float3 F = F_SchlickRoughness(max(dot(H, V), 0.0f), F0, roughness);
       
    // スペキュラの計算
    float3 numerator = NDF * G * F;
    float NdotL = max(dot(N, L), 0.0f);
    float NdotV = max(dot(N, V), 0.0f);
    float denominator = 4.0f * NdotV * NdotL + 0.0001f;
    float3 specular = numerator / denominator;
    
    // エネルギー保存則
    float3 kS = F;
    float3 kD = float3(1.0f, 1.0f, 1.0f) - kS;
    
    // 金属は拡散反射を持たない
    kD *= 1.0f - metalness;

    // 最終合成
    return (kD * albedo / PI + specular) * lightColor * lightIntensity * NdotL;
}