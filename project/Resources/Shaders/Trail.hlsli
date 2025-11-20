#include "ShaderConstants.hlsli"

ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);

struct VertexShaderInput
{
    float32_t4 position : POSITION0;
    float32_t2 texcoord : TEXCOORD0;
    float32_t4 color : COLOR0; 
};

struct VertexShaderOutput
{
    float32_t4 position : SV_POSITION;
    float32_t2 texcoord : TEXCOORD0;
    float32_t4 color : COLOR0;
};