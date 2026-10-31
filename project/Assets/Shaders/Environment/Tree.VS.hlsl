#include "Common/Object3D.hlsli"
#include "Common/ShaderConstants.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<GlobalEnvironmentData> gEnvironmentData : register(b4);
ConstantBuffer<LeafMaterialData> gMaterial : register(b5);
ConstantBuffer<TreeInstanceOffset> gTreeInstanceOffset : register(b9);

StructuredBuffer<TreeInstanceData> gInstanceData : register(t10);
Texture2D<float> gWindMap : register(t11);

SamplerState gLinearWrapSampler : register(s2);

struct TreeVSInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
    float3 tangent : TANGENT0;
};

struct TreePSInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 worldPosition : WORLD_POSITION;
    float ao : COLOR0;            // 根元ほど暗くする
    float lodFade : BLENDWEIGHT;
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

TreePSInput main(TreeVSInput input, uint instanceID : SV_InstanceID)
{
    TreeInstanceData instance = gInstanceData[instanceID + gTreeInstanceOffset.baseInstanceIndex];

    float3 localPosition = input.position.xyz;
    float3 worldPosition = mul(float4(localPosition, 1.0f), instance.worldMatrix).xyz;
    float3 rootPosition = instance.worldMatrix[3].xyz;

    // 根元 0 〜 てっぺん 1
    float heightRatio = saturate((worldPosition.y - rootPosition.y) / max(gMaterial.treeHeight, 0.1f));

    float3 worldNormal = normalize(mul(input.normal, (float3x3) instance.worldMatrix));
    float3 worldTangent = normalize(mul(input.tangent, (float3x3) instance.worldMatrix));

    // 風の強さ。ノイズテクスチャで場所ごとに突風を足す
    float2 windDirection = normalize(gEnvironmentData.windDirection);
    float2 windUV = rootPosition.xz * gMaterial.gustScale - gEnvironmentData.windOffset * gMaterial.windSpeedMultiplier * 0.05f;
    float gust = smoothstep(0.2f, 0.8f, gWindMap.SampleLevel(gLinearWrapSampler, windUV, 0).r);
    float windStrength = gEnvironmentData.windSpeed * (1.0f + gust * gMaterial.gustStrength);

    // 木ごとに揺れのタイミングをずらす
    float treePhase = dot(rootPosition.xz, float2(0.13f, 0.17f));
    float windTime = gEnvironmentData.windTime * gMaterial.windSpeedMultiplier;

    // 幹のしなり。根元は動かさず、上ほど大きく曲げる
    float swayWave = sin(windTime + treePhase) * 0.3f + 0.7f;
    float bendAngle = swayWave * heightRatio * heightRatio * gMaterial.trunkFlexibility * windStrength * 0.15f;
    float3x3 bendRotation = AngleAxisTo3x3(normalize(float3(-windDirection.y, 0.0f, windDirection.x)), bendAngle);

    float3 bentPosition = rootPosition + mul(worldPosition - rootPosition, bendRotation);
    worldNormal = mul(worldNormal, bendRotation);
    worldTangent = mul(worldTangent, bendRotation);

    // 葉だけ細かく揺らす
    if (gTreeInstanceOffset.isLeaf != 0)
    {
        float flutterPhase = dot(localPosition, float3(3.5f, 4.2f, 2.8f)) + treePhase;
        float flutterWave = sin(windTime * 14.0f + flutterPhase) * cos(windTime * 9.0f + flutterPhase * 0.5f);
        bentPosition += worldNormal * flutterWave * gMaterial.leafFlutterAmount * windStrength;
    }

    TreePSInput output;
    output.position = mul(float4(bentPosition, 1.0f), gFrameData.viewProjectionMatrix);
    output.worldPosition = bentPosition;
    output.texcoord = input.texcoord;
    output.normal = worldNormal;
    output.tangent = worldTangent;
    output.ao = lerp(0.3f, 1.0f, saturate(heightRatio * 2.0f));
    output.lodFade = instance.lodFade;
    return output;
}
