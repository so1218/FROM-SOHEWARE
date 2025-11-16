#include "ShaderConstants.hlsli"

ConstantBuffer<TransformationMatrix> gTransform : register(b1);

struct VertexInput
{
    float4 position : POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
};

struct VertexOutput
{
    float4 position : SV_POSITION;
};

VertexOutput main(VertexInput input)
{
    VertexOutput output;
    output.position = mul(input.position, gTransform.WVP);
    return output;
}