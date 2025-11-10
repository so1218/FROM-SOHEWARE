#include "Skybox.hlsli"

struct VertexShaderInput
{
    float3 position : POSITION;
};

struct TransformationMatrix
{
    float32_t4x4 WVP;
};

cbuffer TransformBuffer : register(b1)
{
    TransformationMatrix gTransformationMatrix;
};

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    output.position = mul(float4(input.position, 1.0f), gTransformationMatrix.WVP).xyww;
    output.texcoord = input.position.xyz;
	
	return output;
}