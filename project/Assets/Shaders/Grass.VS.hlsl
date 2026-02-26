#include "Object3D.hlsli"
#include "ShaderConstants.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);
cbuffer DirectionalLights : register(b1)
{
    DirectionalLight gDirectionalLights[MAX_DIRECTIONAL_LIGHTS];
};
ConstantBuffer<MaterialData> gMaterial : register(b5);

StructuredBuffer<GrassInstance> gInstanceData : register(t10);

PixelShaderInput main(VertexShaderInput input, uint instanceID : SV_InstanceID)
{
    PixelShaderInput output;
    
    float4x4 worldMatrix = gInstanceData[instanceID].world;
    float4 instanceColor = gInstanceData[instanceID].color;
    float4 localPos = input.position;

    // 草の揺れ処理
    if (localPos.y > 0.0f)
    {
        float time = gFrameData.gTime * gMaterial.wobbleSpeed;
        // 座標からシードを作って揺れをバラけさせる
        float seed = worldMatrix[3].x + worldMatrix[3].z;
        float waveX = sin(time + seed);
        float waveZ = cos(time * 0.7f + seed); // 周期を少しずらす
        localPos.x += waveX * gMaterial.wobbleAmplitude * localPos.y;
        localPos.z += waveZ * gMaterial.wobbleAmplitude * localPos.y;
    }

    float4 worldPos = mul(localPos, worldMatrix);
    
    output.position = mul(worldPos, gFrameData.viewProjectionMatrix);
    output.texcoord = input.texcoord;
    output.normal = normalize(mul(input.normal, (float3x3) worldMatrix));
    output.worldPosition = worldPos.xyz;
    output.shadowCoord = mul(worldPos, gDirectionalLights[0].viewProj);
    output.tangent = normalize(mul(input.tangent, (float3x3) worldMatrix));
    output.worldColor = instanceColor;
    
    return output;
}