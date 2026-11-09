#include "Common/ShaderConstants.hlsli"

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

ConstantBuffer<MaterialData> gMaterialData : register(b5);

PixelShaderOutput main()
{
    PixelShaderOutput output;
    
    output.color = gMaterialData.outlineColor;
    
    return output;
}