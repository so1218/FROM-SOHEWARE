#include "Common/Object3D.hlsli"
#include "Common/ShaderConstants.hlsli"
#include "Common/MathUtils.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<WaterMaterialData> gWaterMaterial : register(b5);
ConstantBuffer<InstanceOffset> gInstanceOffset : register(b7);
ConstantBuffer<InteractionConstants> gInteractionData : register(b8);

StructuredBuffer<Object3DInstanceData> gInstanceData : register(t10);
Texture2D<float4> gInteractionMap : register(t11);

SamplerState gClampSampler : register(s1);

// 風と物理法則から多重 Gerstner 波をプロシージャル自動計算
float3 CalculateAutoGerstnerWorld(float3 worldPos, float time, out float3 outNormal, out float3 outTangent)
{
    static const int OCTAVES = 4;
    static const float GRAVITY = 9.81f;

    float3 waveOffset = float3(0.0f, 0.0f, 0.0f);
    float3 tangent = float3(1.0f, 0.0f, 0.0f);
    float3 binormal = float3(0.0f, 0.0f, 1.0f);

    float currentAmplitude = gWaterMaterial.baseAmplitude;
    float currentLength = max(gWaterMaterial.baseWaveLength, kEpsilon);
    
    float windLen = length(gWaterMaterial.windDirection);
    float2 mainDir = (windLen > kEpsilon) ? (gWaterMaterial.windDirection / windLen) : float2(1.0f, 0.0f);

    float spreadAngle = gWaterMaterial.waveDirectionSpread;
    float cosAngle = cos(spreadAngle);
    float sinAngle = sin(spreadAngle);
    float2x2 rotMatrix = float2x2(cosAngle, -sinAngle, sinAngle, cosAngle);

    float2 currentDir = mainDir;

    [unroll]
    for (int i = 0; i < OCTAVES; ++i)
    {
        float k = (2.0f * PI) / currentLength;
        float w = sqrt(GRAVITY * k) * gWaterMaterial.waveSpeed;

        float phase = k * dot(currentDir, worldPos.xz) + w * time;
        float c = cos(phase);
        float s = sin(phase);

        float q = (gWaterMaterial.baseSteepness / max(k * currentAmplitude * (float) OCTAVES, kEpsilon)) * gWaterMaterial.waveChop;

        waveOffset.x += currentDir.x * (q * currentAmplitude * c);
        waveOffset.y += currentAmplitude * s;
        waveOffset.z += currentDir.y * (q * currentAmplitude * c);

        float wa = k * currentAmplitude;
        tangent += float3(
            -currentDir.x * currentDir.x * (q * wa * s),
             currentDir.x * (wa * c),
            -currentDir.x * currentDir.y * (q * wa * s)
        );

        binormal += float3(
            -currentDir.x * currentDir.y * (q * wa * s),
             currentDir.y * (wa * c),
            -currentDir.y * currentDir.y * (q * wa * s)
        );

        currentAmplitude *= gWaterMaterial.wavePersistence;
        currentLength /= gWaterMaterial.waveLacunarity;
        
        if (i % 2 == 0)
        {
            currentDir = mul(rotMatrix, currentDir);
        }
        else
        {
            currentDir = mul(transpose(rotMatrix), currentDir);
        }
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

    float4 baseWorldPos = mul(input.position, instance.World);
    float4 prevBaseWorldPos = mul(input.position, instance.PrevWorld);

    float time = gFrameData.gTime;
    float pTime = gFrameData.prevTime;

    float3 waveNormal, waveTangent;
    float3 prevWaveNormal, prevWaveTangent;

    // ゲストナー波の計算
    float3 waveOffset = CalculateAutoGerstnerWorld(baseWorldPos.xyz, time, waveNormal, waveTangent);
    float3 prevWaveOffset = CalculateAutoGerstnerWorld(prevBaseWorldPos.xyz, pTime, prevWaveNormal, prevWaveTangent);

    // ★ 1. プレイヤー水面干渉（スムーズな高さ変形）
    float2 interactUV = (baseWorldPos.xz - gInteractionData.centerWorldPos) / gInteractionData.worldSize + 0.5f;
    if (all(interactUV >= 0.0f) && all(interactUV <= 1.0f))
    {
        float4 interactData = gInteractionMap.SampleLevel(gClampSampler, interactUV, 0);
        
        float sinkForce = interactData.b * gWaterMaterial.interactionSinkForce;
        float waveBulge = interactData.a * gWaterMaterial.interactionBulgeForce;
        
        // 縁に向かって滑らかに減衰させるマスク
        float edgeDist = length(interactUV - 0.5f) * 2.0f;
        float edgeFade = smoothstep(1.0f, 0.7f, edgeDist);

        float heightDelta = (waveBulge - sinkForce) * gWaterMaterial.interactionHeightScale * edgeFade;
        waveOffset.y += heightDelta;
    }

    float3 finalWorldPos = baseWorldPos.xyz + waveOffset;
    float3 prevFinalWorldPos = prevBaseWorldPos.xyz + prevWaveOffset;

    output.worldPosition = finalWorldPos;
    output.position = mul(float4(finalWorldPos, 1.0f), gFrameData.viewProjectionMatrix);
    output.currentClipPos = output.position;
    output.prevClipPos = mul(float4(prevFinalWorldPos, 1.0f), gFrameData.prevViewProj);
    output.texcoord = input.texcoord;
    
    float3x3 worldInvTranspose = (float3x3) instance.WorldInverseTranspose;
    output.normal = normalize(mul(waveNormal, worldInvTranspose));
    output.tangent = normalize(mul(waveTangent, (float3x3) instance.World));
    output.worldColor = instance.WorldColor;

    return output;
}