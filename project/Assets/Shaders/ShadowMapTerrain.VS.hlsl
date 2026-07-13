#include "ShaderConstants.hlsli"

// シャドウ用データ
ConstantBuffer<ShadowData> gShadowData : register(b8);
ConstantBuffer<CascadeConstant> gCascadeConstant : register(b9);

StructuredBuffer<TerrainInstanceData> gTerrainInstances : register(t9);
ConstantBuffer<TerrainSettings> gTerrainSettings : register(b10);
Texture2D<float> gHeightMap : register(t8);
SamplerState gSampler : register(s0);

struct TerrainVSInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
};

struct ShadowVSOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

ShadowVSOutput main(TerrainVSInput input, uint instanceID : SV_InstanceID)
{
    ShadowVSOutput output;
    
    // インスタンスデータの取得
    TerrainInstanceData inst = gTerrainInstances[instanceID];
    
    // ハイトマップから高さを取得し、通常描画と完全に一致
    float heightRatio = gHeightMap.SampleLevel(gSampler, input.texcoord, 0).r - 0.5f;
    input.position.y = heightRatio * gTerrainSettings.maxHeight;
    
    // 地形の頂点をワールド空間へ変換
    float4 worldPos = mul(input.position, inst.World);
    
    // ライト視点のクリップ空間へ変換
    output.position = mul(worldPos, gShadowData.cascadeLightViewProj[gCascadeConstant.cascadeIndex]);
    
    output.texcoord = input.texcoord;
    return output;
}