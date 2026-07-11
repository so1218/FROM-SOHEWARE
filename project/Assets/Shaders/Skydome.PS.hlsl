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
// ★FBMによる雲の「高さ」を一元管理する関数（風のアニメーションもここで一括処理）
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
    
    // ベクトルの正規化
    float3 viewDir = normalize(input.viewDir);
    float3 sunDir = normalize(-gDirectionalLights[0].direction);
    
    // 空のベースグラデーション & 大気散乱
    float skyBlend = pow(max(viewDir.y, 0.0f), gWeather.skyGradientExponent);
    float3 skyColor = lerp(gWeather.horizonColor, gWeather.zenithColor, skyBlend);
    
    float groundBlend = smoothstep(0.0f, -0.1f, viewDir.y);
    skyColor = lerp(skyColor, gWeather.groundColor, groundBlend);
    
    float sunDotBase = saturate(dot(viewDir, sunDir));
    float atmosphereScattering = pow(sunDotBase, 4.0f) * gWeather.sunAtmosphereGlow * max(1.0f - viewDir.y, 0.0f);
    skyColor += gWeather.horizonColor * atmosphereScattering;
    
    // -------------------------------------------------------------
    // 2. 雲のUVおよび「高さ・法線」の計算（★ここから大幅修正）
    // -------------------------------------------------------------
    float viewY = max(viewDir.y, 0.05f);
    float2 cloudUV = (viewDir.xz / viewY) * gWeather.cloudScale;
    
    // 現在位置の雲の高さ（旧 combinedNoise）
    float cloudHeight = GetCloudHeight(cloudUV);
    
    // ★リアルタイム法線計算
    // offsetの値を変えることでディテールの細かさが変わります（0.002〜0.01あたりで調整）
    float2 offset = float2(0.005f, 0.0f);
    float heightR = GetCloudHeight(cloudUV + offset.xy); // 右隣の高さ
    float heightU = GetCloudHeight(cloudUV + offset.yx); // 上隣の高さ
    
    // 高さの差分から法線（傾き）を生成
    // 中間の「0.15f」は bumpScale（凹凸の強さ）です。小さくするほどモクモク感が鋭利になります
    float3 cloudNormal = normalize(float3(cloudHeight - heightR, gWeather.cloudBumpScale, cloudHeight - heightU));
    
    // 雲量を適用
    float cloudAlpha = smoothstep(gWeather.cloudCoverage.x, gWeather.cloudCoverage.x + gWeather.cloudEdgeSoftness, cloudHeight);
    float horizonFade = smoothstep(0.05f, 0.25f, viewDir.y);
    cloudAlpha *= horizonFade;
    
    // 雲の厚み（セルフシャドウ用）
    float cloudThickness = smoothstep(max(0.0f, gWeather.cloudCoverage.x - 0.05f), min(1.0f, gWeather.cloudCoverage.y + 0.15f), cloudHeight);
    
    // -------------------------------------------------------------
    // 3. ★立体的なライティングの計算
    // -------------------------------------------------------------
    // 雲は光を透過・拡散しやすいため、通常のLambertではなく「ハーフランバート」を使います
    // これにより、影が真っ黒に潰れず、GTA5のような柔らかい光の回り込みが生まれます
    float NdotL = dot(cloudNormal, sunDir);
    float halfLambert = saturate(NdotL * 0.5f + 0.5f);
    
    // 太陽が当たる表面の色と、当たらない影の色
    float3 litColor = lerp(gDirectionalLights[0].color.rgb, float3(1.0f, 1.0f, 1.0f), 0.4f) * 1.5f;
    float3 shadowColor = lerp(skyColor * gWeather.cloudShadowDensity, gWeather.cloudAmbientColor, 0.5f);
    
    // 法線の傾きに基づいてライティングをブレンド
    float3 baseCloudColor = lerp(shadowColor, litColor, halfLambert);
    
    // さらに雲自体の「厚み」による自己遮蔽（Self-Occlusion）を乗算し、奥まった部分を暗くする
    baseCloudColor *= lerp(1.0f, 1.0f - gWeather.cloudAbsorption, cloudThickness);
    
// -------------------------------------------------------------
    // 4. 太陽・シルバーライニング・最終合成（微調整）
    // -------------------------------------------------------------
    float sunDot = saturate(dot(viewDir, sunDir));
    float sunHeight = saturate(sunDir.y);
    float3 sunsetTint = lerp(float3(1.0f, 0.3f, 0.05f), float3(1.0f, 1.0f, 1.0f), smoothstep(0.0f, 0.2f, sunHeight));
    
    // 太陽本体の描画（変更なし）
    float sunCore = pow(sunDot, 10000.0f);
    float3 coreColor = lerp(float3(1.0f, 0.8f, 0.5f), float3(1.0f, 0.99f, 0.98f), sunHeight) * 600.0f;
    float sunGlow = pow(sunDot, 5000.0f);
    float3 glowColor = lerp(float3(1.0f, 0.1f, 0.0f), float3(1.0f, 0.9f, 0.7f), sunHeight) * sunsetTint * 240.0f;
    float sunHalo = pow(sunDot, 3000.0f);
    float3 haloColor = lerp(float3(0.8f, 0.2f, 0.0f), float3(1.0f, 0.75f, 0.45f), sunHeight) * sunsetTint * 20.0f;
    float3 totalSun = (sunCore * coreColor) + (sunGlow * glowColor) + (sunHalo * haloColor);
    
    // ★追加: Henyey-Greenstein 位相関数による散乱計算
    // g の値（0.0〜0.99）を変えることで、光の鋭さを調整できます（0.8〜0.9が雲に最適）
    float g = 0.85f;
    float g2 = g * g;
    float hgDenom = 1.0f + g2 - 2.0f * g * sunDot;
    // ゼロ除算や極端な値を防ぐために max を噛ませる
    float hgPhase = (1.0f - g2) / pow(max(hgDenom, 0.001f), 1.5f);
    
    // ★修正: pow から HGベースのシルバーライニングに変更
    // 係数(2.0f)はお好みで調整してください
    float silverLining = hgPhase * 2.0f;
    float translucency = (1.0f - cloudThickness) * cloudAlpha;
    
    // 雲の色にシルバーライニング（前方散乱）を足す
    float3 finalCloudColor = baseCloudColor + (float3(1.0f, 1.0f, 1.0f) * silverLining * translucency);
    
    // 背景の空と雲をブレンド
    float3 skyWithClouds = lerp(skyColor, finalCloudColor, cloudAlpha);
    
    // 雲の厚みに応じて太陽自体を遮蔽
    float sunOcclusion = lerp(1.0f, 0.0f, cloudAlpha * cloudThickness);
    totalSun *= sunOcclusion;
    
    // 最終合成
    float3 finalColor = skyWithClouds + totalSun;
    
    output.color = float4(finalColor, 1.0f);
    return output;
}