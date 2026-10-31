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

struct WaterVSOutput
{
    float4 position : SV_POSITION;
    float3 worldPosition : POSITION1;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
    float3 tangent : TANGENT;
};

static const int kWaveCount = 4;
static const float kGravity = 9.81f;

// 向きと長さの違う Gerstner 波を 4 つ重ねて、水面の高さと法線を求める
float3 CalculateGerstnerWaves(float3 worldPosition, float time, out float3 outNormal, out float3 outTangent)
{
    float3 offset = float3(0.0f, 0.0f, 0.0f);
    float3 tangent = float3(1.0f, 0.0f, 0.0f);
    float3 binormal = float3(0.0f, 0.0f, 1.0f);

    float amplitude = gWaterMaterial.waveAmplitude;
    float wavelength = max(gWaterMaterial.waveLength, 0.001f);
    float steepness = saturate(gWaterMaterial.waveSteepness);

    float windLength = length(gWaterMaterial.globalWindDirection);
    float2 windDirection = (windLength > kEpsilon) ? gWaterMaterial.globalWindDirection / windLength : float2(1.0f, 0.0f);
    float windAngle = atan2(windDirection.y, windDirection.x);

    [unroll]
    for (int waveIndex = 0; waveIndex < kWaveCount; ++waveIndex)
    {
        // 風向きから 0, +spread, -spread, +2spread … と左右交互に広げる
        float angleStep = (float) ((waveIndex + 1) / 2) * ((waveIndex % 2 == 1) ? 1.0f : -1.0f);
        float waveAngle = windAngle + angleStep * gWaterMaterial.waveDirectionSpread;
        float2 direction = float2(cos(waveAngle), sin(waveAngle));

        // 深い水の分散関係: 長い波ほど速く進む
        float waveNumber = 2.0f * PI / wavelength;
        float angularSpeed = sqrt(kGravity * waveNumber) * gWaterMaterial.waveSpeed;

        float sinPhase, cosPhase;
        sincos(waveNumber * dot(direction, worldPosition.xz) + angularSpeed * time, sinPhase, cosPhase);

        // 波の頂点がとがりすぎて裏返らないよう、波の数で割っておく
        float sharpness = steepness / (waveNumber * amplitude * kWaveCount);
        float slope = waveNumber * amplitude;

        offset += float3(
            direction.x * sharpness * amplitude * cosPhase,
            amplitude * sinPhase,
            direction.y * sharpness * amplitude * cosPhase);

        tangent += float3(
            -direction.x * direction.x * sharpness * slope * sinPhase,
             direction.x * slope * cosPhase,
            -direction.x * direction.y * sharpness * slope * sinPhase);

        binormal += float3(
            -direction.x * direction.y * sharpness * slope * sinPhase,
             direction.y * slope * cosPhase,
            -direction.y * direction.y * sharpness * slope * sinPhase);

        amplitude *= gWaterMaterial.waveAmplitudeFalloff;
        wavelength *= gWaterMaterial.waveLengthFalloff;
    }

    outNormal = normalize(cross(binormal, tangent));
    outTangent = normalize(tangent);
    return offset;
}

WaterVSOutput main(Object3DVSInputInstanced input)
{
    Object3DInstanceData instance = gInstanceData[input.instanceID + gInstanceOffset.gBaseInstanceIndex];

    float3 worldPosition = mul(input.position, instance.World).xyz;

    float3 waveNormal;
    float3 waveTangent;
    float3 waveOffset = CalculateGerstnerWaves(worldPosition, gFrameData.gTime, waveNormal, waveTangent);

    // プレイヤーなどが入った所はへこみ、通った跡は盛り上がる。
    // インタラクションの範囲の端に近いほど弱める (範囲の外は 0)
    float2 interactionUV = (worldPosition.xz - gInteractionData.centerWorldPos) / gInteractionData.worldSize + 0.5f;
    float edgeFade = smoothstep(1.0f, 0.7f, length(interactionUV - 0.5f) * 2.0f);
    float4 interaction = gInteractionMap.SampleLevel(gClampSampler, interactionUV, 0);
    float sink = interaction.b * gWaterMaterial.interactionSinkForce;
    float bulge = interaction.a * gWaterMaterial.interactionBulgeForce;
    waveOffset.y += (bulge - sink) * gWaterMaterial.interactionHeightScale * edgeFade;

    worldPosition += waveOffset;

    WaterVSOutput output;
    output.position = mul(float4(worldPosition, 1.0f), gFrameData.viewProjectionMatrix);
    output.worldPosition = worldPosition;
    output.texcoord = input.texcoord;
    output.normal = normalize(mul(waveNormal, (float3x3) instance.WorldInverseTranspose));
    output.tangent = normalize(mul(waveTangent, (float3x3) instance.World));
    return output;
}
