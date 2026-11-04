#include "Common/ShaderConstants.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);

struct LightningVSInput
{
    float4 position : POSITION;
    float2 texcoord : TEXCOORD;
    float brightness : BRIGHTNESS; // 消えていく分と明滅を掛けた明るさ (CPU で決める)
};

struct LightningVSOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
    float brightness : BRIGHTNESS;
};

LightningVSOutput main(LightningVSInput input)
{
    LightningVSOutput output;
    output.position = mul(input.position, gFrameData.viewProjectionMatrix);
    output.texcoord = input.texcoord;
    output.brightness = input.brightness;
    return output;
}
