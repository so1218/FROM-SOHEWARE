#include "Object3D.hlsli"
#include "ShaderConstants.hlsli"

struct VertexShaderInput
{
    float32_t4 position : POSITION0;
    float32_t2 texcoord : TEXCOORD0;
    float32_t3 normal : NORMAL0;
};

ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);
ConstantBuffer<OutlineData> gOutlineData : register(b1);

struct OutlineVertexShaderOutput
{
    float32_t4 position : SV_POSITION;
};

OutlineVertexShaderOutput main(VertexShaderInput input)
{
    OutlineVertexShaderOutput output;

    // 定数バッファから太さを取得
    float32_t outlineWidth = gOutlineData.width;

    // 法線方向に頂点を押し出す
    float32_t4 positions = input.position;
    positions.xyz += input.normal * outlineWidth;

    // 座標変換
    output.position = mul(positions, gTransformationMatrix.WVP);

    return output;
}