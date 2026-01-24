#include "Object3D.hlsli"
#include "ShaderConstants.hlsli"

ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);
ConstantBuffer<MaterialData> gMaterialData : register(b1);
ConstantBuffer<FrameData> gFrameData : register(b2);

struct OutlineVertexShaderOutput
{
    float32_t4 position : SV_POSITION;
};

OutlineVertexShaderOutput main(VertexShaderInput input)
{
    OutlineVertexShaderOutput output;

    // 頂点位置をクリップ空間へ変換
    float4 clipPos = mul(input.position, gTransformationMatrix.WVP);

    // 法線を正規化してクリップ空間へ変換
    float3 normal = normalize(input.smoothNormal);
    float4 clipNormal = mul(float4(normal, 0.0f), gTransformationMatrix.WVP);

    // 画面上でのアウトライン押し出し方向
    float2 offsetDir = normalize(clipNormal.xy);

    // ピクセル単位のNDCサイズを計算
    float2 ndcPixelSize = float2(2.0f, 2.0f) / gFrameData.screenResolution;

    // 深度による過剰な拡大を防ぐための制限
    float depthScale = min(clipPos.w, 20.0f);

    // 解像度と深度に応じたアウトラインオフセット
    float2 offset = offsetDir * ndcPixelSize * gMaterialData.outlineWidth * depthScale;

    // オフセットを適用
    output.position = clipPos;
    output.position.xy += offset;

    return output;
}