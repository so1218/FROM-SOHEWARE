#include "ShaderConstants.hlsli" 

// 空用キューブマップ
TextureCube<float4> gSkyTexture : register(t0);
// 雲用のシームレスな2Dノイズテクスチャ
Texture2D<float4> gCloudTexture : register(t1);

SamplerState gSampler : register(s0);

struct SkydomeVertexShaderOutput
{
    float4 position : SV_Position;
    float3 viewDir : TEXCOORD0;
    float2 uv : TEXCOORD1;
};

struct PixelShaderOutput
{
    float4 color : SV_Target;
};

ConstantBuffer<FrameData> gFrameData : register(b0);
cbuffer DirectionalLights : register(b1)
{
    DirectionalLight gDirectionalLights[MAX_DIRECTIONAL_LIGHTS];
};
ConstantBuffer<MaterialData> gMaterial : register(b5);
ConstantBuffer<WeatherData> gWeather : register(b6);
// FBMによる雲の高さを一元管理する関数（風のアニメーションもここで一括処理）
float GetCloudHeight(float2 baseUV)
{
    float2 speed1 = gWeather.windVelocity;
    float2 speed2 = gWeather.windVelocity * 1.5f + float2(-0.003f, 0.009f);
    float2 speed3 = gWeather.windVelocity * 2.0f + float2(0.004f, -0.002f);
    
    float2 uv1 = baseUV + speed1 * gFrameData.gTime;
    float2 uv2 = (baseUV * 2.0f) + speed2 * gFrameData.gTime;
    float2 uv3 = (baseUV * 4.0f) + speed3 * gFrameData.gTime;

    float noise1 = gCloudTexture.Sample(gSampler, uv1).r;
    float noise2 = gCloudTexture.Sample(gSampler, uv2).r;
    float noise3 = gCloudTexture.Sample(gSampler, uv3).r;

    return (noise1 * 0.6f) + (noise2 * 0.3f) + (noise3 * 0.1f);
}

PixelShaderOutput main(SkydomeVertexShaderOutput input)
{
    PixelShaderOutput output;
    float3 viewDir = normalize(input.viewDir);
    
    // ライティング用のベクトル（昼は太陽、夜は月になる）
    float3 activeLightDir = normalize(-gDirectionalLights[0].direction);
    
    // 空に絵を描くためのベクトル
    float3 sunVisualDir = normalize(-gWeather.sunDirection);
    float3 moonVisualDir = -sunVisualDir; // 月は常に太陽の反対
    
    // 空のベースグラデーション & 大気散乱
    float skyBlend = pow(max(viewDir.y, 0.0f), gWeather.skyGradientExponent);
    float3 skyColor = lerp(gWeather.horizonColor, gWeather.zenithColor, skyBlend);
    
    float groundBlend = smoothstep(0.0f, -0.1f, viewDir.y);
    skyColor = lerp(skyColor, gWeather.groundColor, groundBlend);
    
    float sunDotBase = saturate(dot(viewDir, sunVisualDir));
    float atmosphereScattering = pow(sunDotBase, 4.0f) * gWeather.sunAtmosphereGlow * max(1.0f - viewDir.y, 0.0f);
    skyColor += gWeather.horizonColor * atmosphereScattering;
    
    // 雲のUVおよび高さ・法線の計算
    float viewY = max(viewDir.y, 0.05f);
    float2 cloudUV = (viewDir.xz / viewY) * gWeather.cloudScale;
    
    float cloudHeight = GetCloudHeight(cloudUV);
    
    float2 offset = float2(0.005f, 0.0f);
    float heightR = GetCloudHeight(cloudUV + offset.xy);
    float heightU = GetCloudHeight(cloudUV + offset.yx);
    
    float3 cloudNormal = normalize(float3(cloudHeight - heightR, gWeather.cloudBumpScale, cloudHeight - heightU));
    
    float cloudAlpha = smoothstep(gWeather.cloudCoverage.x, gWeather.cloudCoverage.x + gWeather.cloudEdgeSoftness, cloudHeight);
    float horizonFade = smoothstep(0.05f, 0.25f, viewDir.y);
    cloudAlpha *= horizonFade;
    
    float cloudThickness = smoothstep(max(0.0f, gWeather.cloudCoverage.x - 0.05f), min(1.0f, gWeather.cloudCoverage.y + 0.15f), cloudHeight);
    
    // 立体的なライティングの計算
    // 昼は太陽、夜は月明かりになる
    float NdotL = dot(cloudNormal, activeLightDir);
    float halfLambert = saturate(NdotL * 0.5f + 0.5f);
    
    float3 litColor = lerp(gDirectionalLights[0].color.rgb, float3(1.0f, 1.0f, 1.0f), 0.4f) * 1.5f;
    float3 shadowColor = lerp(skyColor * gWeather.cloudShadowDensity, gWeather.cloudAmbientColor, 0.5f);
    
    float3 baseCloudColor = lerp(shadowColor, litColor, halfLambert);
    baseCloudColor *= lerp(1.0f, 1.0f - gWeather.cloudAbsorption, cloudThickness);
    
    // 太陽と月の見た目の描画と散乱計算
    float sunDot = saturate(dot(viewDir, sunVisualDir));
    float moonDot = saturate(dot(viewDir, moonVisualDir));
    
    // 太陽の描画
    float sunHeight = saturate(sunVisualDir.y);
    float3 sunsetTint = lerp(float3(1.0f, 0.3f, 0.05f), float3(1.0f, 1.0f, 1.0f), smoothstep(0.0f, 0.2f, sunHeight));
    float sunCore = pow(sunDot, 10000.0f);
    float3 coreColor = lerp(float3(1.0f, 0.8f, 0.5f), float3(1.0f, 0.99f, 0.98f), sunHeight) * 600.0f;
    float sunGlow = pow(sunDot, 5000.0f);
    float3 glowColor = lerp(float3(1.0f, 0.1f, 0.0f), float3(1.0f, 0.9f, 0.7f), sunHeight) * sunsetTint * 240.0f;
    float3 totalSun = (sunCore * coreColor) + (sunGlow * glowColor);
    
    // 月の描画
    float moonCore = smoothstep(0.999f, 0.9995f, moonDot);
    float moonGlow = pow(moonDot, 3000.0f) * 2.0f;
    float3 moonColorBase = float3(0.6f, 0.8f, 1.0f);
    float3 totalMoon = (moonCore * 2.0f + moonGlow) * moonColorBase;
    
    // シルバーライニング (Henyey-Greenstein)
    float g = 0.85f;
    float g2 = g * g;
    float translucency = (1.0f - cloudThickness) * cloudAlpha;

    // 太陽の散乱
    float hgDenomSun = 1.0f + g2 - 2.0f * g * sunDot;
    float hgPhaseSun = (1.0f - g2) / pow(max(hgDenomSun, 0.001f), 1.5f);
    float3 sunSilverLining = float3(1.0f, 1.0f, 1.0f) * (hgPhaseSun * 2.0f) * translucency;

    // 月の散乱
    float hgDenomMoon = 1.0f + g2 - 2.0f * g * moonDot;
    float hgPhaseMoon = (1.0f - g2) / pow(max(hgDenomMoon, 0.001f), 1.5f);
    float3 moonSilverLining = moonColorBase * (hgPhaseMoon * 1.0f) * translucency;
    
    // 雲の色に太陽と月の散乱を足す
    float3 finalCloudColor = baseCloudColor + sunSilverLining + moonSilverLining;
    
    // 背景の空と雲をブレンド
    float3 skyWithClouds = lerp(skyColor, finalCloudColor, cloudAlpha);
    
    // 雲の厚みに応じて天体自体を遮蔽
    float skyOcclusion = lerp(1.0f, 0.0f, cloudAlpha * cloudThickness);
    totalSun *= skyOcclusion;
    totalMoon *= skyOcclusion; // 月も雲で遮蔽する
    
    // 最終合成 
    float3 finalColor = skyWithClouds + totalSun + totalMoon;
    
    output.color = float4(finalColor, 1.0f);
    return output;
}