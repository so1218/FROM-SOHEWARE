#include "Common/ShaderConstants.hlsli"

ConstantBuffer<TransformationMatrix> gTransform : register(b0);

struct SkydomeVSInput
{
    float3 position : POSITION;
};

struct SkydomeVSOutput
{
    float4 position : SV_Position;
    float3 viewDirection : TEXCOORD0;
};

SkydomeVSOutput main(SkydomeVSInput input)
{
    SkydomeVSOutput output;

    // z を w にして、深度を画面の一番奥に固定する
    output.position = mul(float4(input.position, 1.0f), gTransform.WVP).xyww;

    // 球の中心から頂点への向きが、そのまま視線の向きになる
    output.viewDirection = input.position;
    return output;
}
