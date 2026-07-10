#include "ShaderConstants.hlsli"

// 地形用のワールド行列
ConstantBuffer<TransformationMatrix> gTransform : register(b6);
// シャドウ用データ（ライトのビュープロジェクション行列など）
ConstantBuffer<ShadowData> gShadowData : register(b8);

ConstantBuffer<CascadeConstant> gCascadeConstant : register(b9);

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

ShadowVSOutput main(TerrainVSInput input)
{
    ShadowVSOutput output;
    
    // 地形の頂点をワールド空間へ変換
    float4 worldPos = mul(input.position, gTransform.World);
    
    // ライト視点のクリップ空間へ変換（これがシャドウマップに書き込まれる深度になります）
    output.position = mul(worldPos, gShadowData.cascadeLightViewProj[gCascadeConstant.cascadeIndex]);
    
    output.texcoord = input.texcoord;
    return output;
}