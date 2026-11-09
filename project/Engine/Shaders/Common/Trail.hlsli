#include "ShaderConstants.hlsli"

ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);
ConstantBuffer<TrailMaterialData> gTrailMaterial : register(b1);
ConstantBuffer<FrameData> gFrameData : register(b2);

struct TrailVSInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float4 color : COLOR0; 
};

struct TrailVSOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float4 color : COLOR0;
    float2 texcoordRaw : TEXCOORD1;
};