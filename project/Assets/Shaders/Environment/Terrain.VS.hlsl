#include "Common/Object3D.hlsli"
#include "Environment/TerrainCommon.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);

struct TerrainVSInput
{
    uint2 gridPosition : POSITION0; // チャンクの中での頂点の位置
};

VertexShaderOutput main(TerrainVSInput input, uint instanceID : SV_InstanceID)
{
    int2 texel = GetTerrainTexel(input.gridPosition, instanceID);
    float3 worldPosition = GetTerrainWorldPosition(texel, LoadTerrainHeight(texel));
    float3 normal = CalculateTerrainNormal(texel);

    VertexShaderOutput output;
    output.position = mul(float4(worldPosition, 1.0f), gFrameData.viewProjectionMatrix);
    output.worldPosition = worldPosition;
    output.texcoord = (float2(texel) + 0.5f) / float2(gTerrainSettings.heightMapWidth, gTerrainSettings.heightMapHeight);
    output.normal = normal;
    // 斜面に沿った X 方向
    output.tangent = normalize(cross(normal, float3(0.0f, 0.0f, 1.0f)));
    output.worldColor = float4(1.0f, 1.0f, 1.0f, 1.0f);
    return output;
}
