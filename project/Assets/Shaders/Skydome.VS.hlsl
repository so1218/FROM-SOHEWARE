#include "ShaderConstants.hlsli"

struct VertexShaderInput
{
    float3 position : POSITION;
    float2 uv : TEXCOORD0; // 天球メッシュが持つ雲用の2D UV座標
};

struct SkydomeVertexShaderOutput
{
    float4 position : SV_Position;
    float3 viewDir : TEXCOORD0; // ピクセルシェーダーでの太陽計算用
    float2 uv : TEXCOORD1; // 雲のスクロール用
};

ConstantBuffer<TransformationMatrix> gTransform : register(b0);

SkydomeVertexShaderOutput main(VertexShaderInput input)
{
    SkydomeVertexShaderOutput output;
    
    // xywwで深度を画面の最奥に固定
    output.position = mul(float4(input.position, 1.0f), gTransform.WVP).xyww;

    // ローカルの中心からの方向がそのまま視線方向ベクトル
    output.viewDir = input.position.xyz;
    
    // 雲用のUVをピクセルシェーダーに渡す
    output.uv = input.uv;
    
    return output;
}