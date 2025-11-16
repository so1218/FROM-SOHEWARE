#include "ShaderConstants.hlsli"

ConstantBuffer<MaterialData> gMaterial : register(b0);

float4 main() : SV_Target
{
    return gMaterial.color;
}