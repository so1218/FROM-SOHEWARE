#include "ShaderConstants.hlsli"

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
    float3 tangent : TANGENT0;
};

struct ShadowVSOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

float3 RotateAboutAxis(float3 pos, float3 axis, float angle)
{
    float s = sin(angle);
    float c = cos(angle);
    return pos * c + cross(axis, pos) * s + axis * dot(axis, pos) * (1.0f - c);
}

ShadowVSOutput main(ShadowVSInput input, uint instanceID : SV_InstanceID)
{
    ShadowVSOutput output;
    
    // 1. オフセットを加算してインスタンスデータを取得
    uint actualIndex = instanceID + gTreeInstanceOffset.baseInstanceIndex;
    TreeInstanceData instance = gInstanceData[actualIndex];
    float isLeaf = (float) gTreeInstanceOffset.isLeaf;
    
    float3 origLocalPos = input.position.xyz;
    float4 localPos = float4(origLocalPos, 1.0f);
    float4 baseWorldPos = mul(localPos, instance.worldMatrix);
    float3 rootPos = instance.worldMatrix[3].xyz;
    
    // 高さと高さ比率
    float currentHeight = max(0.0f, baseWorldPos.y - rootPos.y);
    float heightRatio = saturate(currentHeight / max(gMaterial.treeHeight, 0.1f));
    
    // 風の全体的な強度とマップサンプリング
    float2 windDir = normalize(gEnvironmentData.windDirection);
    float windTime = gEnvironmentData.windTime;
    
    float2 windUV = (rootPos.xz * gMaterial.gustScale) - windDir * windTime * 0.05f;
    float gustNoise = gWindMap.SampleLevel(gLinearWrapSampler, windUV, 0).r;
    float gustMask = smoothstep(0.2f, 0.8f, gustNoise);
    float totalWind = gMaterial.baseWindStrength + (gustMask * gMaterial.gustStrength);
    float treePhase = dot(rootPos.xz, float2(0.13f, 0.17f)) + instance.colorVariation.x * 12.34f;
    
    // =========================================================================
    // ★ 1次風: 幹の「ピボット回転しなり」（メインVSと完全一致）
    // =========================================================================
    float3 rotAxis = normalize(float3(-windDir.y, 0.0f, windDir.x));
    float trunkWeight = heightRatio * heightRatio;
    float mainSway = sin(windTime * 1.0f + treePhase) * 0.3f + 0.7f;
    float subSway = sin(windTime * 1.8f + treePhase * 1.5f) * 0.2f;
    
    float bendAngle = (mainSway + subSway) * trunkWeight * gMaterial.trunkFlexibility * totalWind * 0.15f;
    float3 relWorldPos = baseWorldPos.xyz - rootPos;
    float3 bentRelPos = RotateAboutAxis(relWorldPos, rotAxis, bendAngle);

    // =========================================================================
    // ★ 2次風: 枝のうねり
    // =========================================================================
    float branchDist = length(origLocalPos.xz);
    float branchWeight = saturate(branchDist / max(gMaterial.treeRadius, 0.001f));
    float branchPhase = dot(origLocalPos, float3(0.5f, 0.8f, 0.3f)) + treePhase;
    float branchWave = sin(windTime * 2.5f * gEnvironmentData.windTurbulence + branchPhase);
    float3 branchDir = normalize(float3(windDir.x, -0.2f, windDir.y));
    
    float3 branchOffset = branchDir * branchWave * branchWeight * trunkWeight * gMaterial.branchFlexibility * totalWind * isLeaf;
    
    // =========================================================================
    // ★ 3次風: 葉のチラつき (シャドウでは法線が不要なため、ローカル座標ベースで簡略化計算でもOKです)
    // =========================================================================
  // ★ 3次風: 葉のチラつき（Leaf Flutter / Rustle） - 葉メッシュのみ
    float flutterPhase = dot(origLocalPos, float3(3.5f, 4.2f, 2.8f)) + treePhase;

// ★ windTime に周波数倍率 (leafFlutterFrequency) を掛ける
    float flutterSpeed = windTime * max(gMaterial.leafFlutterFrequency, 0.0f);

// 固定値だった 14.0f や 9.0f に flutterSpeed を使う
    float flutterWave = sin(flutterSpeed * 14.0f + flutterPhase) * cos(flutterSpeed * 9.0f + flutterPhase * 0.5f);
  // 【修正】仮のベクトルではなく、正確なワールド法線を計算する
    float3 worldNormal = normalize(mul(input.normal, (float3x3) instance.worldMatrix));
    
    // 幹の回転（1次風）に合わせて法線も回転させる（本体と影の座標を100%一致させるため）
    worldNormal = RotateAboutAxis(worldNormal, rotAxis, bendAngle);

    float3 flutterOffset = worldNormal
                         * flutterWave
                         * gMaterial.leafFlutterAmount 
                         * totalWind
                         * isLeaf;

    // 最終的なワールド座標
    float3 finalWorldPos = rootPos + bentRelPos + branchOffset + flutterOffset;

    // 影用の行列変換
    output.position = mul(float4(finalWorldPos, 1.0f), gShadowData.cascadeLightViewProj[gCascadeIndex]);
    output.texcoord = input.texcoord;

    return output;
}