#include "Common/ShaderConstants.hlsli"

struct LinePSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR; 
};

ConstantBuffer<MaterialData> gMaterial : register(b0);

float4 main(LinePSInput input) : SV_Target
{
    return input.color;
}