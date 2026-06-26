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
    float2 cloudUV = (viewDir.xz / viewY) * 0.3f;
    
    // スピードとスケールを散らす（視差効果で立体感を出す）
    float2 speed1 = float2(0.006f, 0.003f);
    float2 speed2 = float2(-0.003f, 0.009f);
    float2 speed3 = float2(0.004f, -0.002f);
    
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
    
    // 雲の基本密度（アルファ）
    float cloudAlpha = smoothstep(0.35f, 0.7f, combinedNoise);
    float horizonFade = smoothstep(0.05f, 0.25f, viewDir.y);
    cloudAlpha *= horizonFade;
    
  // ==========================================
    // 新3. 疑似ボリュメトリック陰影（コントラストと立体感の復活）
    // ==========================================
    // ① 影の境界を再調整（少しメリハリを持たせて、モクモクした形を際立たせる）
    float cloudThickness = smoothstep(0.3f, 0.85f, combinedNoise);
    
    // ② 影の色：空の色を環境光として使いつつ、しっかりと暗さを出す（0.85 -> 0.4にダウン）
    // ほんの少しだけライトの逆色（青紫系）を混ぜると、さらに空気感が出ます
    float3 shadowColor = skyColor * 0.6f;
    
    // ③ 光が当たる表面：純白とライトカラーのミックス（ここは維持）
    float3 litColor = lerp(gDirectionalLights[0].color.rgb, float3(1.0f, 1.0f, 1.0f), 0.6f) * 1.2f;
    
    // ④ 陰影の合成：厚みがある部分はしっかり影を落とす（リミッターを 0.7 -> 0.9 に引き上げ）
    float shadowMix = cloudThickness * 0.7f;
    float3 baseCloudColor = lerp(litColor, shadowColor, shadowMix);
    
    // ==========================================
    // 4. 太陽の計算（そのまま維持）
    // ==========================================
    float sunDot = saturate(dot(viewDir, sunDir));
    
    float sunCore = pow(sunDot, 8000.0f);
    float3 coreColor = float3(1.0f, 0.99f, 0.98f) * 300.0f;
    
    float sunGlow = pow(sunDot, 1000.0f);
    float3 glowColor = float3(1.0f, 0.9f, 0.7f) * 30.0f;
    
    float sunHalo = pow(sunDot, 150.0f);
    float3 haloColor = float3(1.0f, 0.75f, 0.45f) * 4.0f;

    float3 totalSun = (sunCore * coreColor) + (sunGlow * glowColor) + (sunHalo * haloColor);
    
    // ==========================================
    // 5. 雲との光の相互作用（純白の透過とシルバーライニング）
    // ==========================================
    float silverLining = pow(sunDot, 64.0f) * 10.0f;
    float translucency = (1.0f - cloudThickness) * cloudAlpha;
    
    // ★修正：オレンジを消し、ピュアホワイト（1.0, 1.0, 1.0）の強烈な光にする
    float3 finalCloudColor = baseCloudColor + (float3(1.0f, 1.0f, 1.0f) * silverLining * translucency);
    
    float3 skyWithClouds = lerp(skyColor, finalCloudColor, cloudAlpha);
    
    float sunOcclusion = lerp(1.0f, 0.0f, cloudAlpha * cloudThickness);
    totalSun *= sunOcclusion;
    
    float3 finalColor = skyWithClouds + totalSun;
    
    output.color = float4(finalColor, 1.0f);
    return output;
}