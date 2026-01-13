#include "Object3D.hlsli"
#include "ShaderConstants.hlsli" 

// オブジェクトの変換行列
ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);

// ライト情報（今は配列先頭をバインドだが複数に対応したい）
ConstantBuffer<DirectionalLight> gLight : register(b1);

struct ShadowVSOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

ShadowVSOutput main(VertexShaderInput input)
{
    ShadowVSOutput output;

    // ワールド座標に変換
    float4 worldPos = mul(input.position, gTransformationMatrix.World);

    // ライト視点の射影行列を適用
    output.position = mul(worldPos, gLight.viewProj);
    
    // UVをパス
    output.texcoord = input.texcoord;

    return output;
}