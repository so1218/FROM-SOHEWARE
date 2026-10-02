#include "Common/ShaderConstants.hlsli"
#include "Common/MathUtils.hlsli"

Texture2D<float4> gCloudTexture : register(t0); 

SamplerState gSampler : register(s0);

struct SkydomeVSOutput
{
    float4 position : SV_Position;
    float3 viewDir : TEXCOORD0;
    float2 uv : TEXCOORD1;
};

struct SkydomePSOutput
{
    float4 color : SV_Target;
};

ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<AtmosphereSkyData> gWeather : register(b6);

cbuffer DirectionalLights : register(b1)
{
    DirectionalLight gDirectionalLights[MAX_DIRECTIONAL_LIGHTS];
};

// 雲のFBM (Fractal Brownian Motion)
// 風速ベクトルで各オクターブをスクロールさせ、時間変化を伴う雲のハイトマップを生成
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

SkydomePSOutput main(SkydomeVSOutput input)
{
    SkydomePSOutput output;
    float3 viewDir = normalize(input.viewDir);
    
    float3 activeLightDir = normalize(-gDirectionalLights[0].direction);
    float3 sunVisualDir = normalize(-gWeather.sunDirection);
    float3 moonVisualDir = -sunVisualDir;
    
    // 空のグラデーションと大気散乱
    float skyBlend = pow(max(viewDir.y, 0.0f), gWeather.skyGradientExponent);
    float3 skyColor = lerp(gWeather.horizonColor, gWeather.zenithColor, skyBlend);
    
    float groundBlend = smoothstep(0.0f, -0.1f, viewDir.y);
    skyColor = lerp(skyColor, gWeather.groundColor, groundBlend);
    
    // レイリー散乱の簡易近似による太陽方向へのグロウ
    float sunDotBase = saturate(dot(viewDir, sunVisualDir));
    float atmosphereScattering = pow(sunDotBase, 4.0f) * gWeather.sunAtmosphereGlow * max(1.0f - viewDir.y, 0.0f);
    skyColor += gWeather.horizonColor * atmosphereScattering;
    
    // 雲の形状と法線
    // 擬似的な地球の曲率を適用し、地平線付近のUVの極端な伸びを抑制
    float viewY = max(viewDir.y, 0.0f);
    float curvedY = sqrt(viewY * viewY + 0.02f);
    float2 cloudUV = (viewDir.xz / curvedY) * gWeather.cloudScale;
    
    float cloudHeight = GetCloudHeight(cloudUV);
    float2 offset = float2(0.005f, 0.0f);
    float heightR = GetCloudHeight(cloudUV + offset.xy);
    float heightU = GetCloudHeight(cloudUV + offset.yx);
    float3 cloudNormal = normalize(float3(cloudHeight - heightR, gWeather.cloudBumpScale, cloudHeight - heightU));
    
    // カバレッジベースの形状決定と地平線フェード
    float cloudAlpha = smoothstep(gWeather.cloudCoverage.x, gWeather.cloudCoverage.x + gWeather.cloudEdgeSoftness, cloudHeight);
    float horizonFade = smoothstep(0.05f, 0.2f, viewDir.y);
    cloudAlpha *= horizonFade;
    
    float cloudThickness = smoothstep(max(0.0f, gWeather.cloudCoverage.x - 0.05f), min(1.0f, gWeather.cloudCoverage.y + 0.15f), cloudHeight);
    
    // 雲のライティングと疑似SSS
    float NdotL = dot(cloudNormal, activeLightDir);
    float halfLambert = saturate(NdotL * 0.5f + 0.5f);
    
    float3 directLightColor = gDirectionalLights[0].color.rgb * gDirectionalLights[0].intensity;
    float3 ambientLight = gWeather.cloudAmbientColor * gWeather.cloudShadowDensity;
    
    // Beer-Lambert則の近似: 吸収率を用いて積乱雲特有の暗い底面を表現
    float transmittance = exp(-cloudThickness * gWeather.cloudAbsorption * 3.0f);
    float3 baseCloudColor = lerp(ambientLight, directLightColor * transmittance, halfLambert);
    
    // 雲のエッジ部分の擬似SSS
    float edgeTranslucency = pow(1.0f - cloudThickness, 2.0f) * cloudAlpha;
    baseCloudColor += directLightColor * edgeTranslucency * 0.5f;

    // 太陽と月の描画
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
    
    // 合成
    float3 skyWithClouds = lerp(skyColor, baseCloudColor, cloudAlpha);
    
    // 雲の厚みに応じた天体のオクルージョン
    float skyOcclusion = lerp(1.0f, 0.0f, cloudAlpha * cloudThickness);
    totalSun *= skyOcclusion;
    totalMoon *= skyOcclusion;
    
    float3 finalColor = skyWithClouds + totalSun + totalMoon;
    
    output.color = float4(finalColor, 1.0f);
    return output;
}