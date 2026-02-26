#include "Object3D.hlsli"
#include "ShaderConstants.hlsli"

cbuffer DirectionalLights : register(b1)
{
    DirectionalLight gDirectionalLights[MAX_DIRECTIONAL_LIGHTS];
};

ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b6);
StructuredBuffer<Well> gMatrixPalette : register(t8);

struct SkinningVertexShaderInput
{
    float32_t4 position : POSITION0;
    float32_t2 texcoord : TEXCOORD0;
    float32_t3 normal : NORMAL0;
    float32_t4 weight : WEIGHT0;
    int32_t4 index : INDEX0;
    float32_t3 tangent : TANGENT;
};

Skinned Skinning(SkinningVertexShaderInput input)
{
    Skinned skinned;

    // 位置のスキニング
    skinned.position =
        mul(input.position, gMatrixPalette[input.index.x].skeletonSpaceMatrix) * input.weight.x +
        mul(input.position, gMatrixPalette[input.index.y].skeletonSpaceMatrix) * input.weight.y +
        mul(input.position, gMatrixPalette[input.index.z].skeletonSpaceMatrix) * input.weight.z +
        mul(input.position, gMatrixPalette[input.index.w].skeletonSpaceMatrix) * input.weight.w;
    skinned.position.w = 1.0f;

    // 法線のスキニング
    skinned.normal =
        mul(input.normal, (float3x3) gMatrixPalette[input.index.x].skeletonSpaceInverseTransposeMatrix) * input.weight.x +
        mul(input.normal, (float3x3) gMatrixPalette[input.index.y].skeletonSpaceInverseTransposeMatrix) * input.weight.y +
        mul(input.normal, (float3x3) gMatrixPalette[input.index.z].skeletonSpaceInverseTransposeMatrix) * input.weight.z +
        mul(input.normal, (float3x3) gMatrixPalette[input.index.w].skeletonSpaceInverseTransposeMatrix) * input.weight.w;
    skinned.normal = normalize(skinned.normal);
    
    // タンジェントのスキニング
    skinned.tangent =
        mul(input.tangent, (float3x3) gMatrixPalette[input.index.x].skeletonSpaceInverseTransposeMatrix) * input.weight.x +
        mul(input.tangent, (float3x3) gMatrixPalette[input.index.y].skeletonSpaceInverseTransposeMatrix) * input.weight.y +
        mul(input.tangent, (float3x3) gMatrixPalette[input.index.z].skeletonSpaceInverseTransposeMatrix) * input.weight.z +
        mul(input.tangent, (float3x3) gMatrixPalette[input.index.w].skeletonSpaceInverseTransposeMatrix) * input.weight.w;
    skinned.tangent = normalize(skinned.tangent);

    return skinned;
}

VertexShaderOutput main(SkinningVertexShaderInput input)
{
    VertexShaderOutput output;
    Skinned skinned = Skinning(input);

    // スキニング結果で変換
    output.position = mul(skinned.position, gTransformationMatrix.WVP);
    output.texcoord = input.texcoord;
    output.normal = normalize(mul(skinned.normal, (float3x3) gTransformationMatrix.WorldInverseTranspose));
    output.tangent = normalize(mul(skinned.tangent, (float3x3) gTransformationMatrix.WorldInverseTranspose));

    // ワールド座標を計算
    float4 worldPos = mul(skinned.position, gTransformationMatrix.World);
    output.worldPosition = worldPos.xyz;

    // シャドウマップ用のライト空間座標
    output.shadowCoord = mul(worldPos, gDirectionalLights[0].viewProj);
    
    output.worldColor = gTransformationMatrix.WorldColor;

    return output;
}