#include "Common/Object3D.hlsli"
#include "Environment/TerrainCommon.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<TerrainSettings> gTerrainSettings : register(b10);

Texture2D<float> gHeightMap : register(t9);
StructuredBuffer<TerrainChunkData> gChunks : register(t10);

struct TerrainVSInput
{
    uint2 gridPosition : POSITION0; // チャンクの中での頂点の位置
};

VertexShaderOutput main(TerrainVSInput input, uint instanceID : SV_InstanceID)
{
    TerrainChunkData chunk = gChunks[instanceID];
    int2 texel = int2(chunk.startX, chunk.startZ) + int2(input.gridPosition);

    float height = LoadTerrainHeight(gHeightMap, gTerrainSettings, texel);
    float3 worldPosition = GetTerrainWorldPosition(gTerrainSettings, texel, height);

    // 両隣の高さの差から、法線と X 方向の接線を求める。両隣は2マス離れている
    float heightLeft = LoadTerrainHeight(gHeightMap, gTerrainSettings, texel + int2(-1, 0));
    float heightRight = LoadTerrainHeight(gHeightMap, gTerrainSettings, texel + int2(1, 0));
    float heightBack = LoadTerrainHeight(gHeightMap, gTerrainSettings, texel + int2(0, -1));
    float heightFront = LoadTerrainHeight(gHeightMap, gTerrainSettings, texel + int2(0, 1));
    float neighborDistance = 2.0f * gTerrainSettings.cellSize;

    VertexShaderOutput output;
    output.position = mul(float4(worldPosition, 1.0f), gFrameData.viewProjectionMatrix);
    output.worldPosition = worldPosition;
    output.texcoord = (float2(texel) + 0.5f) / float2(gTerrainSettings.heightMapWidth, gTerrainSettings.heightMapHeight);
    output.normal = normalize(float3(heightLeft - heightRight, neighborDistance, heightBack - heightFront));
    output.tangent = normalize(float3(neighborDistance, heightRight - heightLeft, 0.0f));
    output.worldColor = float4(1.0f, 1.0f, 1.0f, 1.0f);
    return output;
}
