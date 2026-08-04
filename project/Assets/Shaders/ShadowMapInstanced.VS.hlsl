#include "Object3D.hlsli"
#include "ShaderConstants.hlsli" 

ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<DirectionalLight> gLight : register(b1);
ConstantBuffer<MaterialData> gMaterial : register(b5);
ConstantBuffer<InstanceOffset> gInstanceOffset : register(b7);
ConstantBuffer<ShadowData> gShadowData : register(b8);
ConstantBuffer<CascadeConstant> gCascadeConstant : register(b9);
StructuredBuffer<Object3DInstanceData> gInstanceData : register(t10);

struct ShadowVSOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

ShadowVSOutput main(VertexShaderInput input, uint instanceID : SV_InstanceID)
{
    ShadowVSOutput output;
    
    uint index = gInstanceOffset.gBaseInstanceIndex + instanceID;
    float4x4 worldMatrix = gInstanceData[index].World;

    float4 localPos = input.position;

    // 揺らす処理 (Bubble)
    if (gMaterial.isBubble != 0)
    {
        float time = gFrameData.gTime * gMaterial.wobbleSpeed;
        float wave = sin(time + localPos.y * 5.0f) +
                     cos(time + localPos.z * 5.0f) +
                     sin(time + localPos.x * 5.0f);
        localPos.xyz += input.normal * wave * gMaterial.wobbleAmplitude;
    }

    // World行列を適用
    float4 worldPos = mul(localPos, worldMatrix);

    // ライトビュープロジェクションを適用
    output.position = mul(worldPos, gShadowData.cascadeLightViewProj[gCascadeConstant.cascadeIndex]);
    output.texcoord = input.texcoord;

    return output;
}