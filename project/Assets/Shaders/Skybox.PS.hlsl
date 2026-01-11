#include "Skybox.hlsli" 
#include "ShaderConstants.hlsli" 

TextureCube<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float4 color : SV_Target;
};

ConstantBuffer<MaterialData> gMaterial : register(b0);

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    float32_t4 textureColor = gTexture.Sample(gSampler, input.texcoord);
    
    output.color = textureColor * gMaterial.color;
    
    return output;
}