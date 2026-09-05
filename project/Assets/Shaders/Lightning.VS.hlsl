#include "Common/ShaderConstants.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);
SamplerState gSampler : register(s0);

struct LightningVSInput
{
    float4 position : POSITION;
    float2 texcoord : TEXCOORD;
    float4 color : COLOR; 
};

struct LightningVSOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
    float4 color : COLOR;
    float3 worldPos : TEXCOORD1;
};

LightningVSOutput main(LightningVSInput input)
{
    LightningVSOutput output;
    
    output.position = mul(input.position, gFrameData.viewProjectionMatrix);
    output.texcoord = input.texcoord;
    output.color = input.color;
    output.worldPos = input.position.xyz;
    
    return output;
}