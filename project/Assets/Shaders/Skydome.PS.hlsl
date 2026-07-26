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
// FBMによる雲の高さを一元管理する関数
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
    
    // 光源ベクトル
    float3 activeLightDir = normalize(-gDirectionalLights[0].direction);
    float3 sunVisualDir = normalize(-gWeather.sunDirection);
    float3 moonVisualDir = -sunVisualDir;
    
    // --- 2. 空のベースカラーと大気散乱 ---
    float skyBlend = pow(max(viewDir.y, 0.0f), gWeather.skyGradientExponent);
    float3 skyColor = lerp(gWeather.horizonColor, gWeather.zenithColor, skyBlend);
    
    float groundBlend = smoothstep(0.0f, -0.1f, viewDir.y);
    skyColor = lerp(skyColor, gWeather.groundColor, groundBlend);
    
    float sunDotBase = saturate(dot(viewDir, sunVisualDir));
    float atmosphereScattering = pow(sunDotBase, 4.0f) * gWeather.sunAtmosphereGlow * max(1.0f - viewDir.y, 0.0f);
    skyColor += gWeather.horizonColor * atmosphereScattering;
    
    // --- 3. 雲のUV計算 (★地球の曲率を擬似計算して形を良くする) ---
    // 単純な viewY 割り算ではなく、ドーム状になるようにカーブさせる
    float viewY = max(viewDir.y, 0.0f);
    float curvedY = sqrt(viewY * viewY + 0.02f); // 地平線付近の極端な伸びを防ぐ
    float2 cloudUV = (viewDir.xz / curvedY) * gWeather.cloudScale;
    
    // 雲の高さと法線
    float cloudHeight = GetCloudHeight(cloudUV);
    float2 offset = float2(0.005f, 0.0f);
    float heightR = GetCloudHeight(cloudUV + offset.xy);
    float heightU = GetCloudHeight(cloudUV + offset.yx);
    float3 cloudNormal = normalize(float3(cloudHeight - heightR, gWeather.cloudBumpScale, cloudHeight - heightU));
    
    // 雲のアルファ（形状）と厚み
    float cloudAlpha = smoothstep(gWeather.cloudCoverage.x, gWeather.cloudCoverage.x + gWeather.cloudEdgeSoftness, cloudHeight);
    float horizonFade = smoothstep(0.05f, 0.2f, viewDir.y);
    cloudAlpha *= horizonFade;
    
    float cloudThickness = smoothstep(max(0.0f, gWeather.cloudCoverage.x - 0.05f), min(1.0f, gWeather.cloudCoverage.y + 0.15f), cloudHeight);
    
    // --- 4. 立体的なライティング (★雷雨で暗くするための改善) ---
    float NdotL = dot(cloudNormal, activeLightDir);
    float halfLambert = saturate(NdotL * 0.5f + 0.5f);
    
    // 太陽(ディレクショナルライト)の実際の色と強さを使用
    float3 directLightColor = gDirectionalLights[0].color.rgb * gDirectionalLights[0].intensity;
    
    // 環境光 (cloudShadowDensity で全体の暗さをコントロール)
    float3 ambientLight = gWeather.cloudAmbientColor * gWeather.cloudShadowDensity;
    
    // 光の透過率 (Beer-Lambert則の近似)
    // 雲が厚いほど、また absorption(吸収率) が高いほど太陽光を通さず底が真っ黒になる
    float transmittance = exp(-cloudThickness * gWeather.cloudAbsorption * 3.0f);
    
    // NdotLが高い(太陽側)は透過した光、低い部分は環境光
    float3 baseCloudColor = lerp(ambientLight, directLightColor * transmittance, halfLambert);
    
    // 雲のフチ（エッジ）部分だけ光を透けさせる (擬似サブサーフェススキャタリング)
    float edgeTranslucency = pow(1.0f - cloudThickness, 2.0f) * cloudAlpha;
    baseCloudColor += directLightColor * edgeTranslucency * 0.5f;

    // --- 5. 太陽と月の描画 ---
    float sunDot = saturate(dot(viewDir, sunVisualDir));
    float moonDot = saturate(dot(viewDir, moonVisualDir));
    
    float sunHeight = saturate(sunVisualDir.y);
    float3 sunsetTint = lerp(float3(1.0f, 0.3f, 0.05f), float3(1.0f, 1.0f, 1.0f), smoothstep(0.0f, 0.2f, sunHeight));
    float sunCore = pow(sunDot, 10000.0f);
    float3 coreColor = lerp(float3(1.0f, 0.8f, 0.5f), float3(1.0f, 0.99f, 0.98f), sunHeight) * 600.0f;
    float sunGlow = pow(sunDot, 5000.0f);
    float3 glowColor = lerp(float3(1.0f, 0.1f, 0.0f), float3(1.0f, 0.9f, 0.7f), sunHeight) * sunsetTint * 240.0f;
    float3 totalSun = (sunCore * coreColor) + (sunGlow * glowColor);
    
    float moonCore = smoothstep(0.999f, 0.9995f, moonDot);
    float moonGlow = pow(moonDot, 3000.0f) * 2.0f;
    float3 moonColorBase = float3(0.6f, 0.8f, 1.0f);
    float3 totalMoon = (moonCore * 2.0f + moonGlow) * moonColorBase;
    
    // --- シルバーライニング ---
    float g = 0.85f;
    float g2 = g * g;
    float hgTranslucency = (1.0f - cloudThickness) * cloudAlpha;

    float hgDenomSun = 1.0f + g2 - 2.0f * g * sunDot;
    float hgPhaseSun = (1.0f - g2) / pow(max(hgDenomSun, 0.001f), 1.5f);
    // 太陽の強さに依存させる
    float3 sunSilverLining = directLightColor * (hgPhaseSun * 1.5f) * hgTranslucency;

    float hgDenomMoon = 1.0f + g2 - 2.0f * g * moonDot;
    float hgPhaseMoon = (1.0f - g2) / pow(max(hgDenomMoon, 0.001f), 1.5f);
    float3 moonSilverLining = moonColorBase * (hgPhaseMoon * 1.0f) * hgTranslucency;
    
    float3 finalCloudColor = baseCloudColor + sunSilverLining + moonSilverLining;
    
    // --- 7. 合成 ---
    float3 skyWithClouds = lerp(skyColor, finalCloudColor, cloudAlpha);
    
    float skyOcclusion = lerp(1.0f, 0.0f, cloudAlpha * cloudThickness);
    totalSun *= skyOcclusion;
    totalMoon *= skyOcclusion;
    
    float3 finalColor = skyWithClouds + totalSun + totalMoon;
    
    output.color = float4(finalColor, 1.0f);
    return output;
}