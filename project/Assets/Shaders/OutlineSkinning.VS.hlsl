#include "Object3D.hlsli"
#include "ShaderConstants.hlsli"

struct SkinningVSInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
    float4 weight : WEIGHT0;
    int4 index : INDEX0;
    float3 tangent : TANGENT0;
    float3 smoothNormal : TEXCOORD1;
};

struct OutlineVSOutput
{
    float4 position : SV_POSITION;
};

ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<MaterialData> gMaterialData : register(b5);
ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b6);
StructuredBuffer<Well> gMatrixPalette : register(t0);

// スキニング計算関数
Skinned Skinning(SkinningVSInput input)
{
    Skinned skinned;
    
    // 位置の変換
    skinned.position = mul(input.position, gMatrixPalette[input.index.x].skeletonSpaceMatrix) * input.weight.x;
    skinned.position += mul(input.position, gMatrixPalette[input.index.y].skeletonSpaceMatrix) * input.weight.y;
    skinned.position += mul(input.position, gMatrixPalette[input.index.z].skeletonSpaceMatrix) * input.weight.z;
    skinned.position += mul(input.position, gMatrixPalette[input.index.w].skeletonSpaceMatrix) * input.weight.w;
    skinned.position.w = 1.0f;
    
    // 法線の変換
    skinned.normal = mul(input.normal, (float3x3) gMatrixPalette[input.index.x].skeletonSpaceInverseTransposeMatrix) * input.weight.x;
    skinned.normal += mul(input.normal, (float3x3) gMatrixPalette[input.index.y].skeletonSpaceInverseTransposeMatrix) * input.weight.y;
    skinned.normal += mul(input.normal, (float3x3) gMatrixPalette[input.index.z].skeletonSpaceInverseTransposeMatrix) * input.weight.z;
    skinned.normal += mul(input.normal, (float3x3) gMatrixPalette[input.index.w].skeletonSpaceInverseTransposeMatrix) * input.weight.w;
    skinned.normal = normalize(skinned.normal);
    
    // 接線の計算
    skinned.tangent = mul(input.tangent, (float3x3) gMatrixPalette[input.index.x].skeletonSpaceInverseTransposeMatrix) * input.weight.x;
    skinned.tangent += mul(input.tangent, (float3x3) gMatrixPalette[input.index.y].skeletonSpaceInverseTransposeMatrix) * input.weight.y;
    skinned.tangent += mul(input.tangent, (float3x3) gMatrixPalette[input.index.z].skeletonSpaceInverseTransposeMatrix) * input.weight.z;
    skinned.tangent += mul(input.tangent, (float3x3) gMatrixPalette[input.index.w].skeletonSpaceInverseTransposeMatrix) * input.weight.w;
    skinned.tangent = normalize(skinned.tangent);

    // スムース法線の計算
    skinned.smoothNormal = mul(input.smoothNormal, (float3x3) gMatrixPalette[input.index.x].skeletonSpaceInverseTransposeMatrix) * input.weight.x;
    skinned.smoothNormal += mul(input.smoothNormal, (float3x3) gMatrixPalette[input.index.y].skeletonSpaceInverseTransposeMatrix) * input.weight.y;
    skinned.smoothNormal += mul(input.smoothNormal, (float3x3) gMatrixPalette[input.index.z].skeletonSpaceInverseTransposeMatrix) * input.weight.z;
    skinned.smoothNormal += mul(input.smoothNormal, (float3x3) gMatrixPalette[input.index.w].skeletonSpaceInverseTransposeMatrix) * input.weight.w;
    skinned.smoothNormal = normalize(skinned.smoothNormal);
    
    return skinned;
}

OutlineVSOutput main(SkinningVSInput input)
{
    OutlineVSOutput output;

    // スキニング計算
    Skinned skinned = Skinning(input);

    // ワールド→クリップ空間変換
    float4 clipPos = mul(skinned.position, gTransformationMatrix.WVP);
    
    // 法線を正規化してクリップ空間へ変換
    float3 normal = normalize(skinned.smoothNormal);
    float4 clipNormal = mul(float4(normal, 0.0f), gTransformationMatrix.WVP);

    // スクリーンスペースでの押し出し方向
    float2 offsetDir = normalize(clipNormal.xy);

    // ピクセル単位のNDCサイズ
    float2 ndcPixelSize = float2(2.0f, 2.0f) / gFrameData.screenResolution;

    // 過剰な押し出しを防ぐ深度制限
    float depthScale = min(clipPos.w, 20.0f);

    // アウトライン幅に応じたオフセット
    float2 offset = offsetDir * ndcPixelSize * gMaterialData.outlineWidth * depthScale;

    // 位置にオフセットを適用
    output.position = clipPos;
    output.position.xy += offset;

    return output;
}