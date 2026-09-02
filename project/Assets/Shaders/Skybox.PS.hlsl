#include "Skybox.hlsli" 
#include "ShaderConstants.hlsli" 

ConstantBuffer<MaterialData> gMaterial : register(b0);
TextureCube<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct SkyboxPSOutput
{
    float4 color : SV_Target;
};

SkyboxPSOutput main(SkyboxVSOutput input)
{
    SkyboxPSOutput output;
    
    float4 textureColor = gTexture.Sample(gSampler, input.texcoord);
    
    output.color = textureColor * gMaterial.color;
    
    return output;
}