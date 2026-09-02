#include "Object3D.hlsli"
#include "ShaderConstants.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<MaterialData> gMaterialData : register(b5);
ConstantBuffer<InstanceOffset> gInstanceOffset : register(b7);
StructuredBuffer<Object3DInstanceData> gInstanceData : register(t10);

struct OutlineVSOutput
{
    float4 position : SV_POSITION;
};

OutlineVSOutput main(VertexShaderInput input, uint instanceID : SV_InstanceID)
{
    OutlineVSOutput output;

    // 自分のインスタンスデータを取得
    uint index = gInstanceOffset.gBaseInstanceIndex + instanceID;
    float4x4 worldMatrix = gInstanceData[index].World;
    float4x4 wvp = mul(worldMatrix, gFrameData.viewProjectionMatrix); 

    // 頂点位置をクリップ空間へ
    float4 clipPos = mul(input.position, wvp);

    // 法線の計算
    float3 normal = normalize(input.smoothNormal);
    float4 clipNormal = mul(float4(normal, 0.0f), wvp);

    float2 offsetDir = normalize(clipNormal.xy);
    float2 ndcPixelSize = float2(2.0f, 2.0f) / gFrameData.screenResolution;
    float depthScale = min(clipPos.w, 20.0f);
    float2 offset = offsetDir * ndcPixelSize * gMaterialData.outlineWidth * depthScale;

    output.position = clipPos;
    output.position.xy += offset;

    return output;
}
