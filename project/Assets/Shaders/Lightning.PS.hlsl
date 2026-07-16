#include "ShaderConstants.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<LightningMaterial> gLightningMaterial : register(b1);
SamplerState gSampler : register(s0);

struct VertexOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
    float4 color : COLOR;
    float3 worldPos : TEXCOORD1;
};

float4 main(VertexOutput input) : SV_Target
{
    float distFromCenter = abs(input.texcoord.x - 0.5f) * 2.0f;
    
    // Core計算
    float core = saturate(1.0f - (distFromCenter / gLightningMaterial.coreThickness));
    core = pow(core, gLightningMaterial.corePower);
    
    // Glow計算
    float glow = saturate(1.0f - distFromCenter);
    glow = pow(glow, gLightningMaterial.glowPower);
    
    // アニメーション
    float timeVal = gFrameData.gTime * gLightningMaterial.flickerSpeed;
    float flickerNoise = frac(sin(timeVal + gLightningMaterial.instanceSeed) * 43758.5453f);
    float flicker = lerp(gLightningMaterial.flickerMin, gLightningMaterial.flickerMax, flickerNoise);
    
    // 色合成
    float3 cColor = gLightningMaterial.coreColor * core;
    float3 fColor = gLightningMaterial.fringeColor * glow;
    float3 finalColor = (cColor + fColor) * flicker;
    
    // 高輝度化
    finalColor *= gLightningMaterial.emissiveIntensity;
    
    float alpha = glow * input.color.a * flicker;
    if (alpha < 0.01f)
        discard;
    
    return float4(finalColor, alpha);
}