#include "Common/ShaderConstants.hlsli"

struct LineVSInput
{
    float4 position : POSITION;
    float4 color : COLOR; 
};

struct LineVSOutput
{
    float4 position : SV_POSITION;
    float4 color : COLOR; 
};

ConstantBuffer<TransformationMatrix> gTransform : register(b0);

LineVSOutput main(LineVSInput input)
{
    LineVSOutput output;
    output.position = mul(input.position, gTransform.WVP);
    output.color = input.color;
    return output;
}