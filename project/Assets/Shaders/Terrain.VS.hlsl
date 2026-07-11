#include "Object3D.hlsli"
#include "ShaderConstants.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<TerrainSettings> gTerrainSettings : register(b10); 

struct InstanceBuffer
{
    TerrainInstanceData data[300];
};
ConstantBuffer<InstanceBuffer> gTerrainInstances : register(b6);

// ハイトマップテクスチャ
Texture2D<float> gHeightMap : register(t8);
SamplerState gSampler : register(s0);

struct TerrainVSInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
};

// ==========================================
// 通常描画用 頂点シェーダー
// ==========================================
VertexShaderOutput main(TerrainVSInput input, uint instanceID : SV_InstanceID)
{
    VertexShaderOutput output;
    TerrainInstanceData inst = gTerrainInstances.data[instanceID];
    
    // VTF: ハイトマップから高さを取得し、定数バッファの maxHeight を掛ける
    float heightRatio = gHeightMap.SampleLevel(gSampler, input.texcoord, 0).r - 0.5f;
    input.position.y = heightRatio * gTerrainSettings.maxHeight;

    // ワールド・クリップ座標計算
    float4 worldPos = mul(input.position, inst.World);
    output.worldPosition = worldPos.xyz;
    output.position = mul(worldPos, gFrameData.viewProjectionMatrix);
    output.currentClipPos = output.position;
    output.prevClipPos = mul(worldPos, gFrameData.prevViewProj);
    output.texcoord = input.texcoord;
    
   // ★ 法線のGPU計算の修正
    float offset = gTerrainSettings.texelSize;
    float hL = gHeightMap.SampleLevel(gSampler, input.texcoord + float2(-offset, 0), 0).r * gTerrainSettings.maxHeight;
    float hR = gHeightMap.SampleLevel(gSampler, input.texcoord + float2(offset, 0), 0).r * gTerrainSettings.maxHeight;
    float hD = gHeightMap.SampleLevel(gSampler, input.texcoord + float2(0, offset), 0).r * gTerrainSettings.maxHeight;
    float hU = gHeightMap.SampleLevel(gSampler, input.texcoord + float2(0, -offset), 0).r * gTerrainSettings.maxHeight;
    
    // Y成分(高さ)の変化に対するX/Z方向の距離は 2.0 * cellSize になります
    float3 localNormal = normalize(float3(hL - hR, 2.0f * gTerrainSettings.cellSize, hD - hU));
    
    output.normal = normalize(mul(localNormal, (float3x3) inst.WorldInverseTranspose));
    output.tangent = float3(1, 0, 0);
    output.worldColor = inst.WorldColor;
    
    return output;
}