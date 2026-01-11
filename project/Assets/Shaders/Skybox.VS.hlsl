#include "Skybox.hlsli"
#include "ShaderConstants.hlsli"

struct VertexShaderInput
{
    float3 position : POSITION;
};

ConstantBuffer<TransformationMatrix> gTransform : register(b1);

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    
    output.position = mul(float4(input.position, 1.0f), gTransform.WVP).xyww;
    
    output.texcoord = input.position.xyz;
    
    return output;
}