#include "Object3D.hlsli"
#include "ShaderConstants.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<GlobalEnvironmentData> gEnvironmentData : register(b4);
ConstantBuffer<LeafMaterialData> gMaterial : register(b5);
ConstantBuffer<TreeInstanceOffset> gTreeInstanceOffset : register(b9);
StructuredBuffer<TreeInstanceData> gInstanceData : register(t10);

Texture2D<float> gWindMap : register(t11);
SamplerState gLinearWrapSampler : register(s2);

struct VertexInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
    float3 tangent : TANGENT0;
};

struct PixelInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 worldPosition : WORLD_POSITION;
    float4 color : COLOR0; // GustMask と 擬似AO をPSに渡すために使用
    float3 instanceTint : COLOR1;
    float lodFade : BLENDWEIGHT;
};

// 軸と角度から3x3回転行列を作成する高速関数 (RotateAboutAxis 3回分の代わり)
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

PixelInput main(VertexInput input, uint instanceID : SV_InstanceID)
{
    PixelInput output;
    
    uint realInstanceIndex = instanceID + gTreeInstanceOffset.baseInstanceIndex;
    TreeInstanceData instance = gInstanceData[realInstanceIndex];
    
    float3 origLocalPos = input.position.xyz;
    float4 baseWorldPos = mul(float4(origLocalPos, 1.0f), instance.worldMatrix);
    float3 rootPos = instance.worldMatrix[3].xyz;

    float currentHeight = max(0.0f, baseWorldPos.y - rootPos.y);
    float heightRatio = saturate(currentHeight / max(gMaterial.treeHeight, 0.1f));
    
    bool isLeaf = (gTreeInstanceOffset.isLeaf != 0);
    float pseudoAO = lerp(0.3f, 1.0f, saturate(heightRatio * 2.0f));

    // 風の基本計算
    float2 windDir = normalize(gEnvironmentData.windDirection);
    float windTime = gFrameData.gTime * gEnvironmentData.windSpeed;
    
    float2 windUV = (rootPos.xz * gMaterial.gustScale) - windDir * windTime * 0.05f;
    float gustNoise = gWindMap.SampleLevel(gLinearWrapSampler, windUV, 0).r;
    float gustMask = smoothstep(0.2f, 0.8f, gustNoise);
    float totalWind = gMaterial.baseWindStrength + (gustMask * gMaterial.gustStrength);
    
    float treePhase = dot(rootPos.xz, float2(0.13f, 0.17f)) + instance.colorVariation.x * 12.34f;

    // 1次風: 幹の「ピボット回転しなり」（幹・葉共通）
    float3 rotAxis = normalize(float3(-windDir.y, 0.0f, windDir.x));
    float trunkWeight = heightRatio * heightRatio;
    
    float mainSway = sin(windTime * 1.0f + treePhase) * 0.3f + 0.7f;
    float subSway = sin(windTime * 1.8f + treePhase * 1.5f) * 0.2f;
    float bendAngle = (mainSway + subSway) * trunkWeight * gMaterial.trunkFlexibility * totalWind * 0.15f;
    
    float3 relWorldPos = baseWorldPos.xyz - rootPos;
    float3x3 rotMatrix = AngleAxisTo3x3(rotAxis, bendAngle);
    
    float3 bentRelPos = mul(relWorldPos, rotMatrix);
    float3 worldNormal = normalize(mul(input.normal, (float3x3) instance.worldMatrix));
    float3 worldTangent = normalize(mul(input.tangent, (float3x3) instance.worldMatrix));
    
    worldNormal = mul(worldNormal, rotMatrix);
    worldTangent = mul(worldTangent, rotMatrix);

    // ★ 2次・3次風: 葉（Leaf）の場合のみ計算（幹の描画時はスキップ）
    float3 branchOffset = 0.0f.xxx;
    float3 flutterOffset = 0.0f.xxx;

    if (isLeaf)
    {
        // 2次風: 枝のうねり
        float branchDist = length(origLocalPos.xz);
        float branchWeight = saturate(branchDist / max(gMaterial.treeRadius, 0.001f));
        float branchPhase = dot(origLocalPos, float3(0.5f, 0.8f, 0.3f)) + treePhase;
        float branchWave = sin(windTime * 2.5f * gEnvironmentData.windTurbulence + branchPhase);
        float3 branchDir = normalize(float3(windDir.x, -0.2f, windDir.y));
        branchOffset = branchDir * branchWave * branchWeight * trunkWeight * gMaterial.branchFlexibility * totalWind;

        // 3次風: 葉のチラつき
        float flutterPhase = dot(origLocalPos, float3(3.5f, 4.2f, 2.8f)) + treePhase;
        float flutterSpeed = windTime * max(gMaterial.leafFlutterFrequency, 0.0f);
        float flutterWave = sin(flutterSpeed * 14.0f + flutterPhase) * cos(flutterSpeed * 9.0f + flutterPhase * 0.5f);
        flutterOffset = worldNormal * flutterWave * gMaterial.leafFlutterAmount * totalWind;
    }

    // 最終位置の合成
    float3 finalWorldPos = rootPos + bentRelPos + branchOffset + flutterOffset;

    output.position = mul(float4(finalWorldPos, 1.0f), gFrameData.viewProjectionMatrix);
    output.worldPosition = finalWorldPos;
    output.texcoord = input.texcoord;
    
    output.normal = worldNormal;
    output.tangent = worldTangent;
    // ★ bitangent の出力を削除（修正完了）
    
    output.color = float4(gustMask, 0.0f, 0.0f, pseudoAO);
    output.instanceTint = instance.colorVariation.yzw;
    output.lodFade = instance.lodFade;

    return output;
}