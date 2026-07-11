#include "ShaderConstants.hlsli"

// シャドウ用データ（ライトのビュープロジェクション行列など）
ConstantBuffer<ShadowData> gShadowData : register(b8);
ConstantBuffer<CascadeConstant> gCascadeConstant : register(b9);

// ★ 追加: インスタンスデータ(t9) と 地形パラメータ(b10)、ハイトマップ(t8)
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

// ★ SV_InstanceID を受け取るように修正
ShadowVSOutput main(TerrainVSInput input, uint instanceID : SV_InstanceID)
{
    ShadowVSOutput output;
    
    // ★ インスタンスデータの取得
    TerrainInstanceData inst = gTerrainInstances[instanceID];
    
    // ★ VTF: ハイトマップから高さを取得し、通常描画と完全に一致させる
    float heightRatio = gHeightMap.SampleLevel(gSampler, input.texcoord, 0).r - 0.5f;
    input.position.y = heightRatio * gTerrainSettings.maxHeight;
    
    // 地形の頂点をワールド空間へ変換
    float4 worldPos = mul(input.position, inst.World);
    
    // ライト視点のクリップ空間へ変換
    output.position = mul(worldPos, gShadowData.cascadeLightViewProj[gCascadeConstant.cascadeIndex]);
    
    output.texcoord = input.texcoord;
    return output;
}