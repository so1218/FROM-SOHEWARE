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
    
    // バブルの揺れ処理
    if (gMaterial.isBubble != 0)
    {
        float time = gFrameData.gTime * gMaterial.wobbleSpeed;
        float wave = sin(time + localPos.y * 5.0f) + cos(time + localPos.z * 5.0f) + sin(time + localPos.x * 5.0f);
        localPos.xyz += input.normal * wave * gMaterial.wobbleAmplitude;
    }
    
    // 座標変換
    float4 worldPos = mul(localPos, instance.World);
    output.worldPosition = worldPos.xyz;
    output.position = mul(worldPos, gFrameData.viewProjectionMatrix);
    
    output.texcoord = input.texcoord;
    output.normal = normalize(mul(input.normal, (float3x3) instance.WorldInverseTranspose));
    output.tangent = normalize(mul(input.tangent, (float3x3) instance.World));
    output.worldColor = instance.WorldColor;
    
    return output;
}