#include "Object3D.hlsli"
#include "ShaderConstants.hlsli"

// スキニング用の入力（ボーンウェイト等が必要なため）
struct SkinningVertexShaderInput
{
    float32_t4 position : POSITION0;
    float32_t2 texcoord : TEXCOORD0;
    float32_t3 normal : NORMAL0;
    float32_t4 weight : WEIGHT0;
    int32_t4 index : INDEX0;
    float32_t3 smoothNormal : TANGENT0;
};

struct OutlineVertexShaderOutput
{
    float32_t4 position : SV_POSITION;
};

ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);
StructuredBuffer<Well> gMatrixPalette : register(t0); 
ConstantBuffer<OutlineData> gOutlineData : register(b1);
ConstantBuffer<FrameData> gFrameData : register(b2);

// スキニング計算関数
Skinned Skinning(SkinningVertexShaderInput input)
{
    Skinned skinned;
    
    // 位置の変換
    skinned.position = mul(input.position, gMatrixPalette[input.index.x].skeletonSpaceMatrix) * input.weight.x;
    skinned.position += mul(input.position, gMatrixPalette[input.index.y].skeletonSpaceMatrix) * input.weight.y;
    skinned.position += mul(input.position, gMatrixPalette[input.index.z].skeletonSpaceMatrix) * input.weight.z;
    skinned.position += mul(input.position, gMatrixPalette[input.index.w].skeletonSpaceMatrix) * input.weight.w;
    skinned.position.w = 1.0f;
    
    // 法線の変換
    skinned.normal = mul(input.normal, (float32_t3x3) gMatrixPalette[input.index.x].skeletonSpaceInverseTransposeMatrix) * input.weight.x;
    skinned.normal += mul(input.normal, (float32_t3x3) gMatrixPalette[input.index.y].skeletonSpaceInverseTransposeMatrix) * input.weight.y;
    skinned.normal += mul(input.normal, (float32_t3x3) gMatrixPalette[input.index.z].skeletonSpaceInverseTransposeMatrix) * input.weight.z;
    skinned.normal += mul(input.normal, (float32_t3x3) gMatrixPalette[input.index.w].skeletonSpaceInverseTransposeMatrix) * input.weight.w;
    skinned.normal = normalize(skinned.normal);
    
    skinned.smoothNormal = mul(input.smoothNormal, (float32_t3x3) gMatrixPalette[input.index.x].skeletonSpaceInverseTransposeMatrix) * input.weight.x;
    skinned.smoothNormal += mul(input.smoothNormal, (float32_t3x3) gMatrixPalette[input.index.y].skeletonSpaceInverseTransposeMatrix) * input.weight.y;
    skinned.smoothNormal += mul(input.smoothNormal, (float32_t3x3) gMatrixPalette[input.index.z].skeletonSpaceInverseTransposeMatrix) * input.weight.z;
    skinned.smoothNormal += mul(input.smoothNormal, (float32_t3x3) gMatrixPalette[input.index.w].skeletonSpaceInverseTransposeMatrix) * input.weight.w;
    skinned.smoothNormal = normalize(skinned.smoothNormal);
    
    return skinned;
}

OutlineVertexShaderOutput main(SkinningVertexShaderInput input)
{
    OutlineVertexShaderOutput output;

    // スキニング計算
    Skinned skinned = Skinning(input);

    // 位置をクリップ空間へ
    float4 clipPos = mul(skinned.position, gTransformationMatrix.WVP);

    // 法線をクリップ空間へ 
    float3 normal = normalize(skinned.smoothNormal);
    float4 clipNormal = mul(float4(normal, 0.0f), gTransformationMatrix.WVP);

    // アウトライン押し出し計算
    float2 offsetDir = normalize(clipNormal.xy);
    float2 ndcPixelSize = float2(2.0f, 2.0f) / gFrameData.screenResolution;
    // clipPos.w（深度）が大きくなりすぎないように制限（Clamp）をかける
    float depthScale = min(clipPos.w, 20.0f);

    // クランプした深度を使ってオフセット
    float2 offset = offsetDir * ndcPixelSize * gOutlineData.width * depthScale;

    output.position = clipPos;
    output.position.xy += offset;

    return output;
}