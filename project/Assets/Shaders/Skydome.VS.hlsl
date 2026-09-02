#include "ShaderConstants.hlsli"

ConstantBuffer<TransformationMatrix> gTransform : register(b0);

struct SkydomeVSInput
{
    float3 position : POSITION;
    float2 uv : TEXCOORD0; 
};

struct SkydomeVSOutput
{
    float4 position : SV_Position;
    float3 viewDir : TEXCOORD0; 
    float2 uv : TEXCOORD1; 
};

SkydomeVSOutput main(SkydomeVSInput input)
{
    SkydomeVSOutput output;
    
    // xywwで深度を画面の最奥に固定
    output.position = mul(float4(input.position, 1.0f), gTransform.WVP).xyww;

    // ローカルの中心からの方向がそのまま視線方向ベクトル
    output.viewDir = input.position.xyz;
    
    // 雲用のUVをピクセルシェーダーに渡す
    output.uv = input.uv;
    
    return output;
}