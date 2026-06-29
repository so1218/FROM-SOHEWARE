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

PixelShaderOutput main(SkydomeVertexShaderOutput input)
{
    PixelShaderOutput output;
    
    // ベクトルの正規化
    float3 viewDir = normalize(input.viewDir);
    float3 sunDir = normalize(-gDirectionalLights[0].direction);
    
    // プロシージャルな空のベースグラデーション
    
// 【改良1】グラデーションの制御を smoothstep に変更
    // skyGradientExponent を「ブレンドが完了する高さ」として扱います。
    // 例: 0.3 に設定すると、地平線から少し見上げただけで、一気に完全な天頂の色になります。
    // これにより、地平線の白っぽさが上空を汚すのを防ぎます。
    float skyBlend = smoothstep(0.0f, gWeather.skyGradientExponent, max(viewDir.y, 0.0f));
    
    float3 skyColor = lerp(gWeather.horizonColor, gWeather.zenithColor, skyBlend);
    
    // 地平線より下の処理
    float groundBlend = smoothstep(0.0f, -0.1f, viewDir.y);
    skyColor = lerp(skyColor, gWeather.groundColor, groundBlend);
    
    // 【改良2】大気散乱の簡易シミュレーション
    // 太陽の周りだけを白く/オレンジにする。全体の青空を壊さないように影響範囲を絞る
    float sunDotBase = saturate(dot(viewDir, sunDir));
    float atmosphereScattering = pow(sunDotBase, 16.0f) * gWeather.sunAtmosphereGlow * smoothstep(0.5f, 0.0f, viewDir.y);
    skyColor = lerp(skyColor, gWeather.horizonColor, saturate(atmosphereScattering));
    
    // プロの技：FBMと視差スクロール（立体的な雲の形成）
    float viewY = max(viewDir.y, 0.05f);
    
    float2 cloudUV = (viewDir.xz / viewY) * gWeather.cloudScale;
    
    // 風向きをベースに、レイヤーごとの微小なズレを加算して立体感を出す
    float2 speed1 = gWeather.windVelocity;
    float2 speed2 = gWeather.windVelocity * 1.5f + float2(-0.003f, 0.009f);
    float2 speed3 = gWeather.windVelocity * 2.0f + float2(0.004f, -0.002f);
    
    // レイヤーごとにUVのスケールを変える
    float2 uv1 = cloudUV + speed1 * gFrameData.gTime;
    float2 uv2 = (cloudUV * 2.0f) + speed2 * gFrameData.gTime; // 2倍細かい
    float2 uv3 = (cloudUV * 4.0f) + speed3 * gFrameData.gTime; // 4倍細かい
    
    // 3層のノイズをサンプリング
    float noise1 = gCloudTexture.Sample(gSampler, uv1).r;
    float noise2 = gCloudTexture.Sample(gSampler, uv2).r;
    float noise3 = gCloudTexture.Sample(gSampler, uv3).r;
    
    // FBM合成：ベースの形（大）に、ディテール（中・小）を重ねてフチを複雑にする
    float combinedNoise = (noise1 * 0.6f) + (noise2 * 0.3f) + (noise3 * 0.1f);
    
    // 雲量を適用
    float cloudAlpha = smoothstep(gWeather.cloudCoverage.x, gWeather.cloudCoverage.y, combinedNoise);
    float horizonFade = smoothstep(0.05f, 0.25f, viewDir.y);
    cloudAlpha *= horizonFade;
    
    // 疑似ボリュメトリック陰影
    // 影の境界も雲量に連動
    float cloudThickness = smoothstep(max(0.0f, gWeather.cloudCoverage.x - 0.05f), min(1.0f, gWeather.cloudCoverage.y + 0.15f), combinedNoise);
    
    // 影の濃さを適用
    float3 shadowColor = skyColor * gWeather.cloudShadowDensity;
    
    // 光が当たる表面
    float3 litColor = lerp(gDirectionalLights[0].color.rgb, float3(1.0f, 1.0f, 1.0f), 0.6f) * 1.2f;
    
    // 陰影の合成
    float shadowMix = cloudThickness * 0.7f;
    float3 baseCloudColor = lerp(litColor, shadowColor, shadowMix);
    
    // 太陽の計算
    float sunDot = saturate(dot(viewDir, sunDir));
    
    // 太陽の高さを取得
    float sunHeight = saturate(sunDir.y);
    
    // 太陽が沈むにつれて赤みがかる大気透過率を計算
    float3 sunsetTint = lerp(float3(1.0f, 0.3f, 0.05f), float3(1.0f, 1.0f, 1.0f), smoothstep(0.0f, 0.2f, sunHeight));

    // コア
    float sunCore = pow(sunDot, 8000.0f);
    float3 coreColor = lerp(float3(1.0f, 0.8f, 0.5f), float3(1.0f, 0.99f, 0.98f), sunHeight) * 300.0f;
    
    // グロウ
    float sunGlow = pow(sunDot, 3000.0f);
    float3 glowColor = lerp(float3(1.0f, 0.1f, 0.0f), float3(1.0f, 0.9f, 0.7f), sunHeight) * sunsetTint * 60.0f;
    
    // ハロー
    float sunHalo = pow(sunDot, 1000.0f);
    float3 haloColor = lerp(float3(0.8f, 0.2f, 0.0f), float3(1.0f, 0.75f, 0.45f), sunHeight) * sunsetTint * 4.0f;

    float3 totalSun = (sunCore * coreColor) + (sunGlow * glowColor) + (sunHalo * haloColor);
    
    // 雲との光の相互作用
    float silverLining = pow(sunDot, 64.0f) * 10.0f;
    float translucency = (1.0f - cloudThickness) * cloudAlpha;
    
    float3 finalCloudColor = baseCloudColor + (float3(1.0f, 1.0f, 1.0f) * silverLining * translucency);
    
    float3 skyWithClouds = lerp(skyColor, finalCloudColor, cloudAlpha);
    
    float sunOcclusion = lerp(1.0f, 0.0f, cloudAlpha * cloudThickness);
    totalSun *= sunOcclusion;
    
    float3 finalColor = skyWithClouds + totalSun;
    
    output.color = float4(finalColor, 1.0f);
    return output;
}