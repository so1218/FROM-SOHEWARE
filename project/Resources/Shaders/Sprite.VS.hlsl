#include "ShaderConstants.hlsli" // float32_t などの型定義があればインクルード

// 入力データ (C++側の inputElementsDefault と合わせる)
struct VertexShaderInput
{
    float32_t4 position : POSITION0;
    float32_t2 texcoord : TEXCOORD0;
    float32_t3 normal : NORMAL0; // スプライトでは使わないが、入力レイアウト合わせで定義しておく
};

// 出力データ (PSへ渡す)
struct VertexShaderOutput
{
    float32_t4 position : SV_POSITION;
    float32_t2 texcoord : TEXCOORD0;
};

ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    
    // 座標変換 (WVP行列を掛けるだけ)
    output.position = mul(input.position, gTransformationMatrix.WVP);
    
    // テクスチャ座標はそのまま渡す
    output.texcoord = input.texcoord;
    
    return output;
}