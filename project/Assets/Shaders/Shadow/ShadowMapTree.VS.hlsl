#include "Common/ShaderConstants.hlsli"
#include "Common/MathUtils.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<LeafMaterialData> gMaterial : register(b2);
ConstantBuffer<TreeInstanceOffset> gTreeInstanceOffset : register(b3);
ConstantBuffer<ShadowData> gShadowData : register(b4);
cbuffer cbCascadeIndex : register(b5)
{
    uint gCascadeIndex;
};
ConstantBuffer<GlobalEnvironmentData> gEnvironmentData : register(b6);

StructuredBuffer<TreeInstanceData> gInstanceData : register(t6);
Texture2D<float> gWindMap : register(t7);

SamplerState gLinearWrapSampler : register(s2);

struct ShadowVSInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
};

struct ShadowVSOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

// axis を軸に angle だけ回す行列
float3x3 AngleAxisTo3x3(float3 axis, float angle)
{
    float s, c;
    sincos(angle, s, c);
    float oc = 1.0f - c;

    return float3x3(
        oc * axis.x * axis.x + c, oc * axis.x * axis.y - axis.z * s, oc * axis.z * axis.x + axis.y * s,
        oc * axis.x * axis.y + axis.z * s, oc * axis.y * axis.y + c, oc * axis.y * axis.z - axis.x * s,
        oc * axis.z * axis.x - axis.y * s, oc * axis.y * axis.z + axis.x * s, oc * axis.z * axis.z + c
    );
}

// Tree.VS.hlsl と同じ動きにする
ShadowVSOutput main(ShadowVSInput input, uint instanceID : SV_InstanceID)
{
    TreeInstanceData instance = gInstanceData[instanceID + gTreeInstanceOffset.baseInstanceIndex];

    float3 localPosition = input.position.xyz;
    float3 worldPosition = mul(float4(localPosition, 1.0f), instance.worldMatrix).xyz;
    float3 rootPosition = instance.worldMatrix[3].xyz;

    float heightRatio = saturate((worldPosition.y - rootPosition.y) / max(gMaterial.treeHeight, 0.1f));

    float2 windDirection = normalize(gEnvironmentData.windDirection);
    float2 windUV = rootPosition.xz * gMaterial.gustScale - gEnvironmentData.windOffset * gMaterial.windSpeedMultiplier * 0.05f;
    float gust = smoothstep(0.2f, 0.8f, gWindMap.SampleLevel(gLinearWrapSampler, windUV, 0).r);
    float windStrength = gEnvironmentData.windSpeed * (1.0f + gust * gMaterial.gustStrength);

    float treePhase = dot(rootPosition.xz, float2(0.13f, 0.17f));
    float windTime = gEnvironmentData.windTime * gMaterial.windSpeedMultiplier;

    float swayWave = sin(windTime + treePhase) * 0.3f + 0.7f;
    float bendAngle = swayWave * heightRatio * heightRatio * gMaterial.trunkFlexibility * windStrength * 0.15f;
    float3x3 bendRotation = AngleAxisTo3x3(normalize(float3(-windDirection.y, 0.0f, windDirection.x)), bendAngle);

    float3 bentPosition = rootPosition + mul(worldPosition - rootPosition, bendRotation);

    if (gTreeInstanceOffset.isLeaf != 0)
    {
        float3 worldNormal = normalize(mul(input.normal, (float3x3) instance.worldMatrix));
        worldNormal = mul(worldNormal, bendRotation);

        float flutterPhase = dot(localPosition, float3(3.5f, 4.2f, 2.8f)) + treePhase;
        float flutterWave = sin(windTime * 14.0f + flutterPhase) * cos(windTime * 9.0f + flutterPhase * 0.5f);
        bentPosition += worldNormal * flutterWave * gMaterial.leafFlutterAmount * windStrength;
    }

    ShadowVSOutput output;
    output.position = mul(float4(bentPosition, 1.0f), gShadowData.cascadeLightViewProj[gCascadeIndex]);
    output.texcoord = input.texcoord;
    return output;
}
