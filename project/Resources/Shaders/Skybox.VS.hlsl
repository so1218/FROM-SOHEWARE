#include "Skybox.hlsli"

struct VertexShaderInput
{
    float3 position : POSITION;
};

struct TransformationMatrix
{
    matrix4x4 WVP;
};

cbuffer TransformBuffer : register(b1)
{
    TransformationMatrix gTransformationMatrix;
};

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    output.position = mul(input.position, gTransformationMatrix.WVP).xyww;
    output.texcoord = input.position.xyz;
	
	return output;
}