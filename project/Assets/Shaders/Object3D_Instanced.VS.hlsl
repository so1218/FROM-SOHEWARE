#include "Object3D.hlsli"
#include "ShaderConstants.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);

cbuffer DirectionalLights : register(b1)
{
    DirectionalLight gDirectionalLights[MAX_DIRECTIONAL_LIGHTS];
};

ConstantBuffer<MaterialData> gMaterial : register(b5);
ConstantBuffer<InstanceOffset> gInstanceOffset : register(b7);
StructuredBuffer<Object3DInstanceData> gInstanceData : register(t10);

struct Object3DVSInputInstanced
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
    float3 tangent : TANGENT0;
    float3 smoothNormal : TEXCOORD1;
    uint instanceID : SV_InstanceID;
};

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
    
    // 木の揺れ処理
    if (gMaterial.enableTreeWind != 0)
    {
        // 揺れ始める高さのしきい値
        float thresholdHeight = gMaterial.treeWindThresholdHeight;

        // 現在の計算
        float3 windWorldPos = mul(localPos, instance.World).xyz;
        float phaseOffset = (windWorldPos.x + windWorldPos.z) * gMaterial.treeWindSpatialScale;
        float time = gFrameData.gTime * gMaterial.treeWindSpeed + phaseOffset;

        float waveX = sin(time) * cos(time * 0.45f + phaseOffset);
        float waveZ = cos(time * 0.75f) * sin(time * 0.25f + phaseOffset);
        float normalizedHeight = max(0.0f, (localPos.y - thresholdHeight) * gMaterial.treeWindHeightScale);
  
        localPos.x += waveX * gMaterial.treeWindAmplitude * normalizedHeight;
        localPos.z += waveZ * gMaterial.treeWindAmplitude * normalizedHeight;

        // 過去の計算
        // 風の空間的なズレは過去のワールド座標に依存するため、instance.PrevWorld を使用
        float3 prevWindWorldPos = mul(prevLocalPos, instance.PrevWorld).xyz;
        float prevPhaseOffset = (prevWindWorldPos.x + prevWindWorldPos.z) * gMaterial.treeWindSpatialScale;
        float pTime = gFrameData.prevTime * gMaterial.treeWindSpeed + prevPhaseOffset;

        float pWaveX = sin(pTime) * cos(pTime * 0.45f + prevPhaseOffset);
        float pWaveZ = cos(pTime * 0.75f) * sin(pTime * 0.25f + prevPhaseOffset);
        float pNormalizedHeight = max(0.0f, (prevLocalPos.y - thresholdHeight) * gMaterial.treeWindHeightScale);
  
        prevLocalPos.x += pWaveX * gMaterial.treeWindAmplitude * pNormalizedHeight;
        prevLocalPos.z += pWaveZ * gMaterial.treeWindAmplitude * pNormalizedHeight;
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
    output.normal = normalize(mul(input.normal, (float32_t3x3) instance.WorldInverseTranspose));
    output.tangent = normalize(mul(input.tangent, (float3x3) instance.World));
    output.worldColor = instance.WorldColor;
    
    return output;
}