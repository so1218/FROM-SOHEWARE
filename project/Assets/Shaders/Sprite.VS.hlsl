#include "Common/ShaderConstants.hlsli" 

struct SpriteVSInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0; 
};

struct SpriteVSOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);

SpriteVSOutput main(SpriteVSInput input)
{
    SpriteVSOutput output;

    output.position = mul(input.position, gTransformationMatrix.WVP);

    output.texcoord = input.texcoord;
    
    return output;
}