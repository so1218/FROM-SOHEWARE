#include "Object3D.hlsli"
#include "ShaderConstants.hlsli"

ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);
ConstantBuffer<OutlineData> gOutlineData : register(b1);
ConstantBuffer<FrameData> gFrameData : register(b2);

struct OutlineVertexShaderOutput
{
    float32_t4 position : SV_POSITION;
};

OutlineVertexShaderOutput main(VertexShaderInput input)
{
    OutlineVertexShaderOutput output;

    // クリップ空間へ変換
    float4 clipPos = mul(input.position, gTransformationMatrix.WVP);

    // 法線をクリップ空間へ変換
    float3 normal = normalize(input.smoothNormal);
    float4 clipNormal = mul(float4(normal, 0.0f), gTransformationMatrix.WVP);

    // 画面上の広げる方向（2Dベクトル）
    float2 offsetDir = normalize(clipNormal.xy);

    // スクリーン解像度を使ってオフセット量を計算
    // NDC空間(-1.0～1.0)の幅は2.0
    float2 ndcPixelSize = float2(2.0f, 2.0f) / gFrameData.screenResolution;

    // clipPos.w（深度）が大きくなりすぎないように制限（Clamp）をかける
    float depthScale = min(clipPos.w, 20.0f);

    // 押し出し適用
    // クランプした深度を使ってオフセット
    float2 offset = offsetDir * ndcPixelSize * gOutlineData.width * depthScale;

    output.position = clipPos;
    output.position.xy += offset;

    return output;
}