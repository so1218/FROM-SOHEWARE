#include "Environment/TerrainCommon.hlsli"

ConstantBuffer<ShadowData> gShadowData : register(b8);
ConstantBuffer<CascadeConstant> gCascadeConstant : register(b9);

struct TerrainVSInput
{
    uint2 gridPosition : POSITION0;
};

float4 main(TerrainVSInput input, uint instanceID : SV_InstanceID) : SV_POSITION
{
    int2 texel = GetTerrainTexel(input.gridPosition, instanceID);
    float3 worldPosition = GetTerrainWorldPosition(texel, LoadTerrainHeight(texel));
    return mul(float4(worldPosition, 1.0f), gShadowData.cascadeLightViewProj[gCascadeConstant.cascadeIndex]);
}
