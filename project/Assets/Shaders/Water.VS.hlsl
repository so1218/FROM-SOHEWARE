#include "Object3D.hlsli"
#include "ShaderConstants.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<MaterialData> gMaterial : register(b5);
ConstantBuffer<InstanceOffset> gInstanceOffset : register(b7);
StructuredBuffer<Object3DInstanceData> gInstanceData : register(t10);

// 簡易Gerstner波による頂点変位
float3 CalculateGerstnerWave(float3 pos, float time, float2 dir, float waveLength, float amplitude, float speed)
{
    float w = 2.0f * 3.14159f / waveLength;
    float phi = speed * w;
    float proj = dot(dir, pos.xz);
    float phase = w * proj + phi * time;

    return float3(
        dir.x * (amplitude * cos(phase)),
        amplitude * sin(phase),
        dir.y * (amplitude * cos(phase))
    );
}

VertexShaderOutput main(Object3DVSInputInstanced input)
{
    VertexShaderOutput output;
    
    uint index = input.instanceID + gInstanceOffset.gBaseInstanceIndex;
    Object3DInstanceData instance = gInstanceData[index];

    float4 localPos = input.position;
    float4 prevLocalPos = input.position;

    // 頂点変位
    float time = gFrameData.gTime;
    float pTime = gFrameData.prevTime;

    float3 waveOffset = CalculateGerstnerWave(localPos.xyz, time, float2(1.0f, 0.3f), 12.0f, 0.08f, 1.2f);
    waveOffset += CalculateGerstnerWave(localPos.xyz, time, float2(-0.4f, 0.8f), 6.0f, 0.04f, 1.8f);

    float3 prevWaveOffset = CalculateGerstnerWave(prevLocalPos.xyz, pTime, float2(1.0f, 0.3f), 12.0f, 0.08f, 1.2f);
    prevWaveOffset += CalculateGerstnerWave(prevLocalPos.xyz, pTime, float2(-0.4f, 0.8f), 6.0f, 0.04f, 1.8f);

    localPos.xyz += waveOffset;
    prevLocalPos.xyz += prevWaveOffset;

    // 座標変換 & Velocity構築
    float4 worldPos = mul(localPos, instance.World);
    output.worldPosition = worldPos.xyz;
    output.position = mul(worldPos, gFrameData.viewProjectionMatrix);
    output.currentClipPos = output.position;

    float4 prevWorldPos = mul(prevLocalPos, instance.PrevWorld);
    output.prevClipPos = mul(prevWorldPos, gFrameData.prevViewProj);

    output.texcoord = input.texcoord;
    output.normal = normalize(mul(input.normal, (float3x3) instance.WorldInverseTranspose));
    output.tangent = normalize(mul(input.tangent, (float3x3) instance.World));
    output.worldColor = instance.WorldColor;

    return output;
}