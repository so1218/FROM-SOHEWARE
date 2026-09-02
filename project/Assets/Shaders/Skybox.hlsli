struct SkyboxVSOutput
{
    float4 position : SV_Position;
    float3 texcoord : TEXCOORD0;
    float3 worldPosition : TEXCOORD1;
};