#include "Object3D.hlsli"
#include "ShaderConstants.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<TerrainSettings> gTerrainSettings : register(b10); 

// ハイトマップテクスチャ
Texture2D<float> gTerrainHeightMap : register(t9);
StructuredBuffer<TerrainInstanceData> gTerrainInstances : register(t10);
SamplerState gSampler : register(s0);

struct TerrainVSInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
};

VertexShaderOutput main(TerrainVSInput input, uint instanceID : SV_InstanceID)
{
    VertexShaderOutput output;
    TerrainInstanceData inst = gTerrainInstances[instanceID];

    // インスタンスごとのオフセットを適用して、共通ハイトマップ上のUVを計算
    float2 globalUV = input.texcoord * inst.uvTransform.xy + inst.uvTransform.zw;
    
    // ハイトマップから高さを取得し、maxHeight を掛ける
    float heightRatio = gTerrainHeightMap.SampleLevel(gSampler, globalUV, 0).r - 0.5f;
    input.position.y = heightRatio * gTerrainSettings.maxHeight;

    // ワールド・クリップ座標計算
    float4 worldPos = mul(input.position, inst.World);
    output.worldPosition = worldPos.xyz;
    output.position = mul(worldPos, gFrameData.viewProjectionMatrix);
    output.currentClipPos = output.position;
    output.prevClipPos = mul(worldPos, gFrameData.prevViewProj);
    output.texcoord = input.texcoord;
    
    // 法線のGPU計算
    float offset = gTerrainSettings.texelSize;
    float hL = (gTerrainHeightMap.SampleLevel(gSampler, globalUV + float2(-offset, 0), 0).r - 0.5f) * gTerrainSettings.maxHeight;
    float hR = (gTerrainHeightMap.SampleLevel(gSampler, globalUV + float2(offset, 0), 0).r - 0.5f) * gTerrainSettings.maxHeight;
    float hD = (gTerrainHeightMap.SampleLevel(gSampler, globalUV + float2(0, offset), 0).r - 0.5f) * gTerrainSettings.maxHeight;
    float hU = (gTerrainHeightMap.SampleLevel(gSampler, globalUV + float2(0, -offset), 0).r - 0.5f) * gTerrainSettings.maxHeight;
    
    // 高さの変化に対するX/Z方向の距離は 2.0 * cellSize 
    float3 localNormal = normalize(float3(hL - hR, 2.0f * gTerrainSettings.cellSize, hD - hU));
    
    output.normal = normalize(mul(localNormal, (float3x3) inst.WorldInverseTranspose));
    output.tangent = float3(1, 0, 0);
    output.worldColor = inst.WorldColor;
    
    return output;
}