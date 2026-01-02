#include "ShaderConstants.hlsli"

ConstantBuffer<FrameData> gFrame : register(b0); 

Texture2D<float> depthTexture : register(t0);
SamplerState samplerLinear : register(s0);

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float LinearizeDepth(float d)
{
    return (gFrame.nearClip * gFrame.farClip) / (gFrame.farClip - d * (gFrame.farClip - gFrame.nearClip));
}

float4 main(VSOutput input) : SV_TARGET
{
    float rawDepth = depthTexture.Sample(samplerLinear, input.uv).r;
    float linearDepth = LinearizeDepth(rawDepth);
    float depthValue = linearDepth / gFrame.farClip;

    return float4(depthValue, depthValue, depthValue, 1.0f);
}