#include "Common/Object3D.hlsli"
#include "Common/ShaderConstants.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);

cbuffer DirectionalLights : register(b1)
{
    DirectionalLight gDirectionalLights[MAX_DIRECTIONAL_LIGHTS];
};

ConstantBuffer<MaterialData> gMaterial : register(b5);
ConstantBuffer<InstanceOffset> gInstanceOffset : register(b7);

StructuredBuffer<Object3DInstanceData> gInstanceData : register(t10);

VertexShaderOutput main(Object3DVSInputInstanced input)
{
    VertexShaderOutput output;
    
    uint index = input.instanceID + gInstanceOffset.gBaseInstanceIndex;
    Object3DInstanceData instance = gInstanceData[index];
    
    float4 localPos = input.position;
    // 過去のローカル座標用変数
    float4 prevLocalPos = input.position;
    
    // バブルの揺れ処理
    if (gMaterial.isBubble != 0)
    {
        // 現在の計算
        float time = gFrameData.gTime * gMaterial.wobbleSpeed;
        float wave = sin(time + localPos.y * 5.0f) + cos(time + localPos.z * 5.0f) + sin(time + localPos.x * 5.0f);
        localPos.xyz += input.normal * wave * gMaterial.wobbleAmplitude;

        // 過去の計算
        float pTime = gFrameData.prevTime * gMaterial.wobbleSpeed;
        float pWave = sin(pTime + prevLocalPos.y * 5.0f) + cos(pTime + prevLocalPos.z * 5.0f) + sin(pTime + prevLocalPos.x * 5.0f);
        prevLocalPos.xyz += input.normal * pWave * gMaterial.wobbleAmplitude;
    }
    
    // 座標の最終決定とクリップ空間への変換
    // 現在の座標計算
    float4 worldPos = mul(localPos, instance.World);
    output.worldPosition = worldPos.xyz;
    output.position = mul(worldPos, gFrameData.viewProjectionMatrix);
    output.currentClipPos = output.position;

    // 1フレーム前の座標計算
    float4 prevWorldPos = mul(prevLocalPos, instance.PrevWorld);
    output.prevClipPos = mul(prevWorldPos, gFrameData.prevViewProj);
    
    output.texcoord = input.texcoord;
    output.normal = normalize(mul(input.normal, (float3x3) instance.WorldInverseTranspose));
    output.tangent = normalize(mul(input.tangent, (float3x3) instance.World));
    output.worldColor = instance.WorldColor;
    
    return output;
}