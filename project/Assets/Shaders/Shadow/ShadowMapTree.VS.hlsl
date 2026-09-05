#include "Common/ShaderConstants.hlsli"

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

// メインパスと共通の回転行列生成関数
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

ShadowVSOutput main(ShadowVSInput input, uint instanceID : SV_InstanceID)
{
    ShadowVSOutput output;
    
    uint actualIndex = instanceID + gTreeInstanceOffset.baseInstanceIndex;
    TreeInstanceData instance = gInstanceData[actualIndex];
    
    float3 origLocalPos = input.position.xyz;
    float4 baseWorldPos = mul(float4(origLocalPos, 1.0f), instance.worldMatrix);
    float3 rootPos = instance.worldMatrix[3].xyz;
    
    float currentHeight = max(0.0f, baseWorldPos.y - rootPos.y);
    float heightRatio = saturate(currentHeight / max(gMaterial.treeHeight, 0.1f));
    
    bool isLeaf = (gTreeInstanceOffset.isLeaf != 0);
    
    // -------------------------------------------------------------------------
    // 風のグローバルパラメーター取得
    // -------------------------------------------------------------------------
    float2 windDir = normalize(gEnvironmentData.windDirection);
    float windTime = gEnvironmentData.windTime * gMaterial.windSpeedMultiplier;
    float currentWindMag = gEnvironmentData.windSpeed * gMaterial.windStrengthMultiplier;
    
    float2 windOffset = gEnvironmentData.windOffset * gMaterial.windSpeedMultiplier;
    float2 windUV = (rootPos.xz * gMaterial.gustScale) - windOffset * 0.05f;
    float gustNoise = gWindMap.SampleLevel(gLinearWrapSampler, windUV, 0).r;
    float gustMask = smoothstep(0.2f, 0.8f, gustNoise);
    float totalWind = currentWindMag + (gustMask * gMaterial.gustStrength * gEnvironmentData.windSpeed);
    
    float treePhase = dot(rootPos.xz, float2(0.13f, 0.17f)) + instance.colorVariation.x * 12.34f;
    
    // -------------------------------------------------------------------------
    // 1次風: 幹全体のしなり
    // -------------------------------------------------------------------------
    float3 rotAxis = normalize(float3(-windDir.y, 0.0f, windDir.x));
    float trunkWeight = heightRatio * heightRatio;
    
    float mainSway = sin(windTime * 1.0f + treePhase) * 0.3f + 0.7f;
    float subSway = sin(windTime * 1.8f + treePhase * 1.5f) * 0.2f;
    float bendAngle = (mainSway + subSway) * trunkWeight * gMaterial.trunkFlexibility * totalWind * 0.15f;
    
    float3 relWorldPos = baseWorldPos.xyz - rootPos;
    float3x3 rotMatrix = AngleAxisTo3x3(rotAxis, bendAngle);
    float3 bentRelPos = mul(relWorldPos, rotMatrix);

    // -------------------------------------------------------------------------
    // 2次・3次風: 枝葉の微細な揺れ
    // -------------------------------------------------------------------------
    float3 branchOffset = 0.0f.xxx;
    float3 flutterOffset = 0.0f.xxx;

    // isLeaf を用いた分岐で幹描画時の不要な波計算を完全にスキップ
    if (isLeaf)
    {
        // 枝のうねり
        float branchDist = length(origLocalPos.xz);
        float branchWeight = saturate(branchDist / max(gMaterial.treeRadius, 0.001f));
        float branchPhase = dot(origLocalPos, float3(0.5f, 0.8f, 0.3f)) + treePhase;
        
        float branchWave = sin(windTime * 2.5f + branchPhase);
        float3 branchDir = normalize(float3(windDir.x, -0.2f, windDir.y));
        float turbulenceAmp = max(gEnvironmentData.windTurbulence, 0.5f);
        
        branchOffset = branchDir * branchWave * branchWeight * trunkWeight * gMaterial.branchFlexibility * totalWind * turbulenceAmp;
        
        // 葉のチラつき
        // メインパスと同一の頂点変位を適用
        float3 worldNormal = normalize(mul(input.normal, (float3x3) instance.worldMatrix));
        worldNormal = mul(worldNormal, rotMatrix);

        float flutterPhase = dot(origLocalPos, float3(3.5f, 4.2f, 2.8f)) + treePhase;
        float flutterSpeed = windTime * max(gMaterial.leafFlutterFrequency, 0.0f);
        float flutterWave = sin(flutterSpeed * 14.0f + flutterPhase) * cos(flutterSpeed * 9.0f + flutterPhase * 0.5f);
        
        flutterOffset = worldNormal * flutterWave * gMaterial.leafFlutterAmount * totalWind;
    }

    float3 finalWorldPos = rootPos + bentRelPos + branchOffset + flutterOffset;

    output.position = mul(float4(finalWorldPos, 1.0f), gShadowData.cascadeLightViewProj[gCascadeIndex]);
    output.texcoord = input.texcoord;

    return output;
}