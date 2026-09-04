#include "Object3D.hlsli"
#include "ShaderConstants.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<MaterialData> gMaterial : register(b5);
ConstantBuffer<InstanceOffset> gInstanceOffset : register(b7);
StructuredBuffer<Object3DInstanceData> gInstanceData : register(t10);

struct GerstnerWave
{
    float2 direction;
    float waveLength;
    float amplitude;
    float speed;
    float steepness;
};

// Gerstner波の合成計算 (位置と法線・接線変位)
float3 CalculateMultiGerstner(float3 pos, float time, out float3 outNormal, out float3 outTangent)
{
    static const int WAVE_COUNT = 4;
    GerstnerWave waves[WAVE_COUNT] =
    {
        { normalize(float2(1.0f, 0.3f)), 16.0f, 0.12f, 1.2f, 0.6f },
        { normalize(float2(-0.4f, 0.8f)), 8.0f, 0.06f, 1.8f, 0.5f },
        { normalize(float2(0.2f, -1.0f)), 4.0f, 0.03f, 2.4f, 0.4f },
        { normalize(float2(-0.8f, -0.5f)), 2.0f, 0.01f, 3.1f, 0.3f }
    };

    float3 waveOffset = 0.0f.xxx;
    float3 tangent = float3(1.0f, 0.0f, 0.0f);
    float3 binormal = float3(0.0f, 0.0f, 1.0f);

    [unroll]
    for (int i = 0; i < WAVE_COUNT; ++i)
    {
        float w = 2.0f * PI / waves[i].waveLength;
        float phi = waves[i].speed * w;
        float proj = dot(waves[i].direction, pos.xz);
        float phase = w * proj + phi * time;

        float c = cos(phase);
        float s = sin(phase);
        float q = waves[i].steepness / (w * waves[i].amplitude * (float) WAVE_COUNT);

        waveOffset.x += waves[i].direction.x * (waves[i].amplitude * c);
        waveOffset.y += waves[i].amplitude * s;
        waveOffset.z += waves[i].direction.y * (waves[i].amplitude * c);

        tangent += float3(
            -waves[i].direction.x * waves[i].direction.x * q * s,
            waves[i].direction.x * c * (w * waves[i].amplitude),
            -waves[i].direction.x * waves[i].direction.y * q * s
        );

        binormal += float3(
            -waves[i].direction.x * waves[i].direction.y * q * s,
            waves[i].direction.y * c * (w * waves[i].amplitude),
            -waves[i].direction.y * waves[i].direction.y * q * s
        );
    }

    outNormal = normalize(cross(binormal, tangent));
    outTangent = normalize(tangent);
    return waveOffset;
}

VertexShaderOutput main(Object3DVSInputInstanced input)
{
    VertexShaderOutput output;
    
    uint index = input.instanceID + gInstanceOffset.gBaseInstanceIndex;
    Object3DInstanceData instance = gInstanceData[index];

    float4 localPos = input.position;
    float4 prevLocalPos = input.position;

    float time = gFrameData.gTime;
    float pTime = gFrameData.prevTime;

    float3 waveNormal, waveTangent;
    float3 prevWaveNormal, prevWaveTangent;

    float3 waveOffset = CalculateMultiGerstner(localPos.xyz, time, waveNormal, waveTangent);
    float3 prevWaveOffset = CalculateMultiGerstner(prevLocalPos.xyz, pTime, prevWaveNormal, prevWaveTangent);

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
    
    // 変位後の波の頂点法線・接線を使用
    float3x3 worldInvTranspose = (float3x3) instance.WorldInverseTranspose;
    output.normal = normalize(mul(waveNormal, worldInvTranspose));
    output.tangent = normalize(mul(waveTangent, (float3x3) instance.World));
    output.worldColor = instance.WorldColor;

    return output;
}