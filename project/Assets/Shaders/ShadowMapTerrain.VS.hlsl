#include "ShaderConstants.hlsli"

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
    
    // 構造体全体を読み込まず、必要なプロパティだけを直接フェッチ
    float4 uvTransform = gTerrainInstances[instanceID].uvTransform;
    
    // メイン描画と同じようにUVオフセットを適用
    float2 globalUV = input.texcoord * uvTransform.xy + uvTransform.zw;
    float heightRatio = gHeightMap.SampleLevel(gSampler, globalUV, 0).r - 0.5f;
    input.position.y = heightRatio * gTerrainSettings.maxHeight;
    
    // 行列を丸ごと読み込まず、平行移動成分だけを読み込んで足す
    float3 offset = float3(
        gTerrainInstances[instanceID].World._m30,
        gTerrainInstances[instanceID].World._m31,
        gTerrainInstances[instanceID].World._m32
    );
    float4 worldPos = input.position;
    worldPos.xyz += offset;
    
    // ライト視点のクリップ空間へ変換
    output.position = mul(worldPos, gShadowData.cascadeLightViewProj[gCascadeConstant.cascadeIndex]);
    
    output.texcoord = input.texcoord;
    return output;
}