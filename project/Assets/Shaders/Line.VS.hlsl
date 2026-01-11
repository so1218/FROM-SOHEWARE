#include "ShaderConstants.hlsli"

struct VertexInput
{
    float4 position : POSITION;
    float4 color : COLOR; 
};

struct VertexOutput
{
    float4 position : SV_POSITION;
    float4 color : COLOR; 
};

ConstantBuffer<TransformationMatrix> gTransform : register(b0);

VertexOutput main(VertexInput input)
{
    VertexOutput output;
    output.position = mul(input.position, gTransform.WVP);
    output.color = input.color;
    return output;
}