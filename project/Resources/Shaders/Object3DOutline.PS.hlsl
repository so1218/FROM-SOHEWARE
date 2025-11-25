#include "ShaderConstants.hlsli"

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

ConstantBuffer<OutlineData> gOutlineData : register(b1);

PixelShaderOutput main()
{
    PixelShaderOutput output;
    // アウトラインの色
    output.color = gOutlineData.color;
    return output;
}