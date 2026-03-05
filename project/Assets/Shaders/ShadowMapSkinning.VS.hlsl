#include "Object3D.hlsli"
#include "ShaderConstants.hlsli"

// ライトのビュー射影行列
ConstantBuffer<DirectionalLight> gLight : register(b1);
ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b6);

// スキニング行列
StructuredBuffer<Well> gMatrixPalette : register(t0);

struct SkinningVertexShaderInput
{
    float32_t4 position : POSITION0;
    float32_t2 texcoord : TEXCOORD0;
    float32_t3 normal : NORMAL0;
    float32_t4 weight : WEIGHT0;
    int32_t4 index : INDEX0;
};

struct ShadowVSOutput
{
    float32_t4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

// 位置のみをスキニング
float4 SkinningPosition(float4 pos, float4 weight, int4 index)
{
    float4 skinnedPos =
        mul(pos, gMatrixPalette[index.x].skeletonSpaceMatrix) * weight.x +
        mul(pos, gMatrixPalette[index.y].skeletonSpaceMatrix) * weight.y +
        mul(pos, gMatrixPalette[index.z].skeletonSpaceMatrix) * weight.z +
        mul(pos, gMatrixPalette[index.w].skeletonSpaceMatrix) * weight.w;

    skinnedPos.w = 1.0f;
    return skinnedPos;
}

ShadowVSOutput main(SkinningVertexShaderInput input)
{
    ShadowVSOutput output;

    // スキニング
    float4 skinnedPos = SkinningPosition(input.position, input.weight, input.index);

    // ワールド変換
    float4 worldPos = mul(skinnedPos, gTransformationMatrix.World);

    // ライト空間へ変換
    output.position = mul(worldPos, gLight.viewProj);
    
    // UVをパス
    output.texcoord = input.texcoord;

    return output;
}