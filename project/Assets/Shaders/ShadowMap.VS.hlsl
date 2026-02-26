#include "Object3D.hlsli"
#include "ShaderConstants.hlsli" 

ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<MaterialData> gMaterial : register(b5);
ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b6);
// ライト情報（今は配列先頭をバインドだが複数に対応したい）
ConstantBuffer<DirectionalLight> gLight : register(b7);

struct ShadowVSOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

ShadowVSOutput main(VertexShaderInput input)
{
    ShadowVSOutput output;
    
    float4 localPos = input.position;

    // メインパスと同じ計算で頂点を揺らす
    if (gMaterial.isBubble != 0)
    {
        float time = gFrameData.gTime * gMaterial.wobbleSpeed;
        
        float wave = sin(time + localPos.y * 5.0f) +
                     cos(time + localPos.z * 5.0f) +
                     sin(time + localPos.x * 5.0f);
        
        localPos.xyz += input.normal * wave * gMaterial.wobbleAmplitude;
    }

    // 揺らしたあとのlocalPosを使う
    float4 worldPos = mul(localPos, gTransformationMatrix.World);

    // ライト視点の射影行列を適用
    output.position = mul(worldPos, gLight.viewProj);
    
    // UVをパス
    output.texcoord = input.texcoord;

    return output;
}