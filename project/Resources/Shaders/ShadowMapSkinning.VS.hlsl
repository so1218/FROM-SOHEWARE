#include "Object3D.hlsli"
#include "ShaderConstants.hlsli"

// b0: TransformationMatrix
ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);

// b1: DirectionalLight (ライトのViewProj)
ConstantBuffer<DirectionalLight> gLight : register(b1);

// t0: MatrixPalette (スキニング行列)
StructuredBuffer<Well> gMatrixPalette : register(t0);

struct SkinningVertexShaderInput
{
    float32_t4 position : POSITION0;
    float32_t2 texcoord : TEXCOORD0; // 使わないがInputLayout合わせで必要
    float32_t3 normal : NORMAL0; // 使わないがInputLayout合わせで必要
    float32_t4 weight : WEIGHT0;
    int32_t4 index : INDEX0;
};

struct ShadowVSOutput
{
    float32_t4 position : SV_POSITION;
};

// ▼▼▼ 修正: 位置だけを計算する専用関数 ▼▼▼
float4 SkinningPosition(float4 pos, float4 weight, int4 index)
{
    float4 skinnedPos;
    
    // 位置の変換のみを行う（法線計算は削除）
    skinnedPos = mul(pos, gMatrixPalette[index.x].skeletonSpaceMatrix) * weight.x;
    skinnedPos += mul(pos, gMatrixPalette[index.y].skeletonSpaceMatrix) * weight.y;
    skinnedPos += mul(pos, gMatrixPalette[index.z].skeletonSpaceMatrix) * weight.z;
    skinnedPos += mul(pos, gMatrixPalette[index.w].skeletonSpaceMatrix) * weight.w;
    skinnedPos.w = 1.0f;

    return skinnedPos;
}

ShadowVSOutput main(SkinningVertexShaderInput input)
{
    ShadowVSOutput output;

    // 1. スキニング座標変換 (修正した関数を呼ぶ)
    float4 skinnedPos = SkinningPosition(input.position, input.weight, input.index);

    // 2. ワールド座標変換
    float4 worldPos = mul(skinnedPos, gTransformationMatrix.World);

    // 3. ライト空間へ変換 (シャドウマップ用)
    output.position = mul(worldPos, gLight.viewProj);

    return output;
}