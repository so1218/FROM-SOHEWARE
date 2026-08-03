#include "ShaderConstants.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<LeafMaterialData> gMaterial : register(b2); // ★シャドウのバインドスロットに合わせて調整(例:b2)
cbuffer cbStartInstance : register(b3)
{
    uint gStartInstanceLocation;
};
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

// ==============================================================================
// 頂点シェーダー (葉・幹 共通)
// メインのVSと「全く同じ」揺れ計算を行います。
// ==============================================================================
ShadowVSOutput main(ShadowVSInput input, uint instanceID : SV_InstanceID)
{
    ShadowVSOutput output;
    
    // 1. オフセットを加算してインスタンスデータを取得
    TreeInstanceData instance = gInstanceData[instanceID + gStartInstanceLocation];
    
    // ★ input.position は float4 なので .xyz を取得
    float3 origLocalPos = input.position.xyz;
    float4 localPos = float4(origLocalPos, 1.0f);
    float4 baseWorldPos = mul(localPos, instance.worldMatrix);
    
    // 平行移動成分は 4行目 (Row 3) から取得
    float3 rootPos = instance.worldMatrix[3].xyz;
    
    // メインVSと完全に同じ「ワールド座標ベース」のウェイト計算
    float currentHeight = baseWorldPos.y - rootPos.y;
    
    float trunkWeight = pow(saturate(currentHeight / max(gMaterial.treeHeight, 0.1f)), 1.5f);
    float branchWeight = saturate(length(baseWorldPos.xz - rootPos.xz) / max(gMaterial.treeRadius, 0.001f));
    float leafWeight = gMaterial.isLeaf;

    // -------------------------------------------------------------------------
    // 風の全体的な強度とマップサンプリング
    // -------------------------------------------------------------------------
    float2 windDir = normalize(gEnvironmentData.windDirection);
    float windTime = gFrameData.gTime * gEnvironmentData.windSpeed;
    
    float2 windUV = (rootPos.xz * gMaterial.gustScale) - windDir * windTime * 0.05f;
    float gustNoise = gWindMap.SampleLevel(gLinearWrapSampler, windUV, 0).r;
    float gustMask = smoothstep(0.2f, 0.8f, gustNoise);
    float totalWind = gMaterial.baseWindStrength + (gustMask * gMaterial.gustStrength);
    float treePhase = dot(rootPos.xz, float2(0.1f, 0.1f)) + instance.colorVariation.x * 10.0f;
    
    // -------------------------------------------------------------------------
    // オフセット計算 (メインVSと完全一致させる)
    // -------------------------------------------------------------------------
    float globalWave = sin(windTime * 1.0f + treePhase) * 0.5f + 0.5f;
    float3 trunkOffset = float3(windDir.x, 0.0f, windDir.y) * globalWave * trunkWeight * gMaterial.trunkFlexibility * totalWind;
    
    float branchPhase = origLocalPos.x * 0.5f + origLocalPos.y * 0.5f + origLocalPos.z * 0.5f;
    float branchWave = sin(windTime * 2.5f * gEnvironmentData.windTurbulence + treePhase + branchPhase);
    float3 branchDir = normalize(float3(windDir.x, -0.5f, windDir.y));
    float3 branchOffset = branchDir * branchWave * branchWeight * trunkWeight * gMaterial.branchFlexibility * totalWind;
    
    float flutterPhase = dot(origLocalPos, float3(3.0f, 3.0f, 3.0f));
    float flutterWave = sin(windTime * 15.0f + flutterPhase) * cos(windTime * 11.0f + flutterPhase * 0.5f);
    
    // 法線をベクトル×行列でワールド空間へ変換してから揺らす
    float3 worldNormal = normalize(mul(input.normal, (float3x3) instance.worldMatrix));
    float3 flutterOffset = worldNormal * flutterWave * leafWeight * gMaterial.leafFlutterAmount * totalWind;
    
    // すべてのオフセットを加算
    float3 totalOffset = trunkOffset + branchOffset + flutterOffset;
    
    // Arc Preservation
    float offsetLengthXZ = length(totalOffset.xz);
    totalOffset.y -= offsetLengthXZ * currentHeight * 0.1f;
    
    // 最終的なワールド座標
    float3 finalWorldPos = baseWorldPos.xyz + totalOffset;

    // -------------------------------------------------------------------------
    // 影用の行列変換 (Cascade Shadow)
    // -------------------------------------------------------------------------
    output.position = mul(float4(finalWorldPos, 1.0f), gShadowData.cascadeLightViewProj[gCascadeIndex]);
    output.texcoord = input.texcoord;

    return output;
}