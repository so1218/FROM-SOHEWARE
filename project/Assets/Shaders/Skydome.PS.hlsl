#include "ShaderConstants.hlsli" 

// t0: 既存の空用キューブマップ（グラデーション画像など）
TextureCube<float4> gSkyTexture : register(t0);
// t1: 【新設】雲用のシームレスな2Dノイズテクスチャ（Perlinノイズなど）
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
    
    // 1. ベースの空の色
    float3 skyColor = gSkyTexture.Sample(gSampler, viewDir).rgb * gMaterial.color.rgb;
    
    // ==========================================
    // 2. プロの技：FBMと視差スクロール（立体的な雲の形成）
    // ==========================================
    float viewY = max(viewDir.y, 0.05f);
    
    // ★ C++から受け取ったスケールを適用
    float2 cloudUV = (viewDir.xz / viewY) * gWeather.cloudScale;
    
    // ★ C++から受け取った風向きをベースに、レイヤーごとの視差効果（微小なズレ）を加算して立体感を出す
    float2 speed1 = gWeather.windVelocity;
    float2 speed2 = gWeather.windVelocity * 1.5f + float2(-0.003f, 0.009f);
    float2 speed3 = gWeather.windVelocity * 2.0f + float2(0.004f, -0.002f);
    
    // レイヤーごとにUVのスケール（細かさ）を変える
    float2 uv1 = cloudUV + speed1 * gFrameData.gTime;
    float2 uv2 = (cloudUV * 2.0f) + speed2 * gFrameData.gTime; // 2倍細かい
    float2 uv3 = (cloudUV * 4.0f) + speed3 * gFrameData.gTime; // 4倍細かい
    
    // 3層のノイズをサンプリング
    float noise1 = gCloudTexture.Sample(gSampler, uv1).r;
    float noise2 = gCloudTexture.Sample(gSampler, uv2).r;
    float noise3 = gCloudTexture.Sample(gSampler, uv3).r;
    
    // FBM合成：ベースの形（大）に、ディテール（中・小）を重ねてフチを複雑にする
    float combinedNoise = (noise1 * 0.6f) + (noise2 * 0.3f) + (noise3 * 0.1f);
    
    // ★ C++から受け取った雲量（閾値）を適用
    float cloudAlpha = smoothstep(gWeather.cloudCoverage.x, gWeather.cloudCoverage.y, combinedNoise);
    float horizonFade = smoothstep(0.05f, 0.25f, viewDir.y);
    cloudAlpha *= horizonFade;
    
    // ==========================================
    // 新3. 疑似ボリュメトリック陰影（コントラストと立体感の復活）
    // ==========================================
    // ★ 影の境界も雲量に連動させる（少し広げることで厚みを維持）
    float cloudThickness = smoothstep(max(0.0f, gWeather.cloudCoverage.x - 0.05f), min(1.0f, gWeather.cloudCoverage.y + 0.15f), combinedNoise);
    
    // ★ C++から受け取った影の濃さを適用
    float3 shadowColor = skyColor * gWeather.cloudShadowDensity;
    
    // ③ 光が当たる表面：純白とライトカラーのミックス
    float3 litColor = lerp(gDirectionalLights[0].color.rgb, float3(1.0f, 1.0f, 1.0f), 0.6f) * 1.2f;
    
    // ④ 陰影の合成：完全に影の色（1.0）になりきらないよう、リミッター（* 0.7f）をかける
    float shadowMix = cloudThickness * 0.7f;
    float3 baseCloudColor = lerp(litColor, shadowColor, shadowMix);
    
    // ==========================================
    // 4. 太陽の計算（大気散乱フェイクによる夕焼け自動化）
    // ==========================================
    float sunDot = saturate(dot(viewDir, sunDir));
    
    // 太陽の高さ（Y方向）を取得。1.0=真上、0.0=水平線
    float sunHeight = saturate(sunDir.y);
    
    // 太陽が沈むにつれて赤みがかる「大気透過率」をフェイク計算
    // 高さ0.2以下から急激に赤・オレンジになる
    float3 sunsetTint = lerp(float3(1.0f, 0.3f, 0.05f), float3(1.0f, 1.0f, 1.0f), smoothstep(0.0f, 0.2f, sunHeight));

    // コア（昼は真っ白、夕方は少しオレンジ）
    float sunCore = pow(sunDot, 8000.0f);
    float3 coreColor = lerp(float3(1.0f, 0.8f, 0.5f), float3(1.0f, 0.99f, 0.98f), sunHeight) * 300.0f;
    
    // グロウ（昼は黄色、夕方は強烈な赤外色）
    float sunGlow = pow(sunDot, 3000.0f);
    float3 glowColor = lerp(float3(1.0f, 0.1f, 0.0f), float3(1.0f, 0.9f, 0.7f), sunHeight) * sunsetTint * 60.0f;
    
    // ハロー（光の広がり）
    float sunHalo = pow(sunDot, 1000.0f);
    float3 haloColor = lerp(float3(0.8f, 0.2f, 0.0f), float3(1.0f, 0.75f, 0.45f), sunHeight) * sunsetTint * 4.0f;

    float3 totalSun = (sunCore * coreColor) + (sunGlow * glowColor) + (sunHalo * haloColor);
    
    // ==========================================
    // 5. 雲との光の相互作用（純白の透過とシルバーライニング）
    // ==========================================
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