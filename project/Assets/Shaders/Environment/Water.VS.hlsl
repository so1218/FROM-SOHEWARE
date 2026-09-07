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
float3 CalculateGerstnerWaves(float3 worldPos, float time, out float3 outNormal, out float3 outTangent);

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

    // 波の変位計算
    float3 waveOffset = CalculateGerstnerWaves(baseWorldPos.xyz, time, waveNormal, waveTangent);
    float3 prevWaveOffset = CalculateGerstnerWaves(prevBaseWorldPos.xyz, pTime, prevWaveNormal, prevWaveTangent);

    // プレイヤー水面干渉（インタラクション）
    float2 interactUV = (baseWorldPos.xz - gInteractionData.centerWorldPos) / gInteractionData.worldSize + 0.5f;
    
    // 分岐をなくし、UV範囲外はstep関数でマスクをかけて0にする
    float boundsMask = step(0.0f, interactUV.x) * step(interactUV.x, 1.0f) * step(0.0f, interactUV.y) * step(interactUV.y, 1.0f);
    
    float4 interactData = gInteractionMap.SampleLevel(gClampSampler, interactUV, 0);
    float sinkForce = interactData.b * gWaterMaterial.interactionSinkForce;
    float waveBulge = interactData.a * gWaterMaterial.interactionBulgeForce;
    
    float edgeDist = length(interactUV - 0.5f) * 2.0f;
    float edgeFade = smoothstep(1.0f, 0.7f, edgeDist) * boundsMask;

    waveOffset.y += (waveBulge - sinkForce) * gWaterMaterial.interactionHeightScale * edgeFade;

    // 最終座標の確定
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

float3 CalculateGerstnerWaves(float3 worldPos, float time, out float3 outNormal, out float3 outTangent)
{
    // ループ回数（パフォーマンスに応じて調整、通常3〜4で十分）
    static const int NUM_WAVES = 4;
    static const float GRAVITY = 9.81f;

    float3 waveOffset = float3(0.0f, 0.0f, 0.0f);
    float3 tangent = float3(1.0f, 0.0f, 0.0f);
    float3 binormal = float3(0.0f, 0.0f, 1.0f);

    float amplitude = gWaterMaterial.waveAmplitude;
    float wavelength = max(gWaterMaterial.waveLength, 0.001f);
    float steepness = clamp(gWaterMaterial.waveSteepness, 0.0f, 1.0f);
    
    // ベースとなる風向き
    float windLen = length(gWaterMaterial.globalWindDirection);
    float2 baseDir = (windLen > 0.0001f) ? (gWaterMaterial.globalWindDirection / windLen) : float2(1.0f, 0.0f);

    // 波の拡散用回転行列
    float spread = gWaterMaterial.waveDirectionSpread;
    float2x2 rotMatrix = float2x2(cos(spread), -sin(spread), sin(spread), cos(spread));
    float2 currentDir = baseDir;

    [unroll]
    for (int i = 0; i < NUM_WAVES; ++i)
    {
        // 波数(k) と 角周波数(w)
        float k = (2.0f * PI) / wavelength;
        float w = sqrt(GRAVITY * k) * gWaterMaterial.waveSpeed;

        float phase = k * dot(currentDir, worldPos.xz) + (w * time);
        float sinP, cosP;
        sincos(phase, sinP, cosP);

        // 波が交差してループ（自己交差）しないようにSteepnessを調整
        float q = steepness / (k * amplitude * (float) NUM_WAVES);

        float wa = k * amplitude;
        float xDiff = currentDir.x * (q * amplitude * cosP);
        float zDiff = currentDir.y * (q * amplitude * cosP);
        float yDiff = amplitude * sinP;

        waveOffset += float3(xDiff, yDiff, zDiff);

        // 法線・接線用の偏微分係数
        tangent += float3(
            -currentDir.x * currentDir.x * (q * wa * sinP),
             currentDir.x * (wa * cosP),
            -currentDir.x * currentDir.y * (q * wa * sinP)
        );

        binormal += float3(
            -currentDir.x * currentDir.y * (q * wa * sinP),
             currentDir.y * (wa * cosP),
            -currentDir.y * currentDir.y * (q * wa * sinP)
        );

        // 次のオクターブへの減衰
        amplitude *= gWaterMaterial.waveAmplitudeFalloff;
        wavelength *= gWaterMaterial.waveLengthFalloff;
        
        // オクターブ毎に波の進行方向を交互に振る
        currentDir = mul(i % 2 == 0 ? rotMatrix : transpose(rotMatrix), currentDir);
    }

    outNormal = normalize(cross(binormal, tangent));
    outTangent = normalize(tangent);
    
    return waveOffset;
}