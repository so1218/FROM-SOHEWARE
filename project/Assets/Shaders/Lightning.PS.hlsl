#include "ShaderConstants.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<LightningMaterial> gLightningMaterial : register(b1);
SamplerState gSampler : register(s0);

struct LightningVSOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
    float4 color : COLOR;
    float3 worldPos : TEXCOORD1;
};

// 疑似乱数生成用のハッシュ乗数
// サイン波の周期性を破壊してランダムな値を取り出すための定数
static const float kPrngHashMultiplier = 43758.5453f;

// アルファテストの閾値（この値未満の透明度のピクセルは描画を破棄）
static const float kAlphaClipThreshold = 0.01f;

float4 main(LightningVSOutput input) : SV_Target
{
    // UVのX座標(0.0 ~ 1.0)を、中心(0.5)からの距離(1.0 ~ 0.0 ~ 1.0)に変換
    float distFromCenter = abs(input.texcoord.x - 0.5f) * 2.0f;
    
    // 中心の白く飛ぶ芯の計算
    float core = saturate(1.0f - (distFromCenter / gLightningMaterial.coreThickness));
    core = pow(core, gLightningMaterial.corePower);
    
    // 周囲にフワッと広がる発光の計算
    float glow = saturate(1.0f - distFromCenter);
    glow = pow(glow, gLightningMaterial.glowPower);
    
    // 時間とインスタンスIDに基づく明滅アニメーション
    float timeVal = gFrameData.gTime * gLightningMaterial.flickerSpeed;
    float flickerNoise = frac(sin(timeVal + gLightningMaterial.instanceSeed) * kPrngHashMultiplier);
    float flicker = lerp(gLightningMaterial.flickerMin, gLightningMaterial.flickerMax, flickerNoise);
    
    // 芯と発光をそれぞれのカラーで着色し、明滅を適用
    float3 coreColor = gLightningMaterial.coreColor * core;
    float3 glowColor = gLightningMaterial.fringeColor * glow;
    float3 finalColor = (coreColor + glowColor) * flicker;
    
    // ブルームエフェクトを乗せるための高輝度化
    finalColor *= gLightningMaterial.emissiveIntensity;
  
    // 最終的な透明度の決定
    float alpha = glow * input.color.a * flicker;
    
    // アルファテスト: kAlphaClipThreshold を下回る場合はピクセルを破棄
    clip(alpha - kAlphaClipThreshold);
    
    return float4(finalColor, alpha);
}