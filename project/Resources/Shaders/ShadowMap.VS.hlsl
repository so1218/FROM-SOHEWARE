#include "Object3D.hlsli"
#include "ShaderConstants.hlsli" 

// b0: オブジェクト行列
ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);

// b1: ライト情報 (単体として受け取る例)
// ※C++側で、gDirectionalLights[0] のデータをここにコピーして渡すか、
//  配列の先頭アドレスをここに合わせてバインドする
ConstantBuffer<DirectionalLight> gLight : register(b1);

struct ShadowVSOutput
{
    float32_t4 position : SV_POSITION;
};

ShadowVSOutput main(VertexShaderInput input)
{
    ShadowVSOutput output;
    float32_t4 worldPos = mul(input.position, gTransformationMatrix.World);
    
    // 構造体の中に追加した行列を使う
    output.position = mul(worldPos, gLight.viewProj);
    
    return output;
}