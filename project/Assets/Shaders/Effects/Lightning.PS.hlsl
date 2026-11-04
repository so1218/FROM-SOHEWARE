#include "Common/ShaderConstants.hlsli"

ConstantBuffer<LightningMaterial> gLightningMaterial : register(b1);

struct LightningVSOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
    float brightness : BRIGHTNESS;
};

// 加算で描くので、色にアルファを掛けたものが画面に足される
float4 main(LightningVSOutput input) : SV_Target
{
    // 板の中心が 0、両端が 1
    float distanceFromCenter = abs(input.texcoord.x - 0.5f) * 2.0f;

    // 白く飛ぶ芯と、その周りの青いにじみ
    float core = pow(saturate(1.0f - distanceFromCenter / gLightningMaterial.coreThickness), gLightningMaterial.corePower);
    float glow = pow(saturate(1.0f - distanceFromCenter), gLightningMaterial.glowPower);

    // ブルームで光って見えるように明るくする
    float3 color = (gLightningMaterial.coreColor * core + gLightningMaterial.fringeColor * glow) * gLightningMaterial.emissiveIntensity;

    return float4(color, glow * input.brightness);
}
