#include "Object3D.hlsli"
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

StructuredBuffer<TreeInstanceData> gInstanceData : register(t6);
Texture2D<float> gWindMap : register(t7);
SamplerState gLinearWrapSampler : register(s2);

struct ShadowVSInput
{
    float3 position : POSITION;
    float3 normal : NORMAL; // シャドウでは使わないが入力シグネチャ合わせのために残す
    float4 tangent : TANGENT; // メインVSと合わせる
    float2 texcoord : TEXCOORD;
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
ShadowVSOutput TreeShadowVS(ShadowVSInput input, uint instanceID : SV_InstanceID)
{
    ShadowVSOutput output;
    
    // DrawIndexedInstancedのStartInstanceLocationを加算
    TreeInstanceData instance = gInstanceData[instanceID + gStartInstanceLocation];
    
    float4x4 worldMat = instance.worldMatrix;
    float3 origLocalPos = input.position;
    float3 rootPos = float3(worldMat[0][3], worldMat[1][3], worldMat[2][3]);
    
    // -------------------------------------------------------------------------
    // メインVSと全く同じウェイト計算
    // -------------------------------------------------------------------------
    float trunkWeight = pow(saturate(origLocalPos.y / gMaterial.treeHeight), 1.5f);
    float branchWeight = saturate(length(origLocalPos.xz) / max(gMaterial.treeRadius, 0.001f));
    float leafWeight = (float) gMaterial.isLeaf;

    // -------------------------------------------------------------------------
    // 1. 風の全体的な強度とマップサンプリング
    // -------------------------------------------------------------------------
    float2 windDir = normalize(gMaterial.windDir);
    float windTime = gFrameData.gTime * gMaterial.windSpeed;
    
    float2 windUV = (rootPos.xz * gMaterial.gustScale) - windDir * windTime * 0.05f;
    float gustNoise = gWindMap.SampleLevel(gLinearWrapSampler, windUV, 0).r;
    float gustMask = smoothstep(0.2f, 0.8f, gustNoise);
    float totalWind = gMaterial.baseWindStrength + (gustMask * gMaterial.gustStrength);

    float treePhase = dot(rootPos.xz, float2(0.1f, 0.1f)) + instance.colorVariation.x * 10.0f;
    
    // -------------------------------------------------------------------------
    // 2. AAA級 オフセット計算 (メインVSと完全一致)
    // -------------------------------------------------------------------------
    float globalWave = sin(windTime * 1.0f + treePhase) * 0.5f + 0.5f;
    float3 trunkOffset = float3(windDir.x, 0.0f, windDir.y) * globalWave * trunkWeight * gMaterial.trunkFlexibility * totalWind;
    
    float branchPhase = origLocalPos.x * 0.5f + origLocalPos.y * 0.5f + origLocalPos.z * 0.5f;
    float branchWave = sin(windTime * 2.5f * gMaterial.windTurbulence + treePhase + branchPhase);
    
    float3 branchDir = normalize(float3(windDir.x, -0.5f, windDir.y));
    float3 branchOffset = branchDir * branchWave * branchWeight * trunkWeight * gMaterial.branchFlexibility * totalWind;
    
    float3 displacedPos = origLocalPos + trunkOffset + branchOffset;
    
    float origLen = length(origLocalPos);
    if (origLen > 0.001f)
    {
        displacedPos = normalize(displacedPos) * origLen;
    }
    
    float flutterPhase = dot(origLocalPos, float3(3.0f, 3.0f, 3.0f));
    float flutterWave = sin(windTime * 15.0f + flutterPhase) * cos(windTime * 11.0f + flutterPhase * 0.5f);
    
    // input.normal をそのまま使う（揺れの方向用）
    float3 flutterOffset = input.normal * flutterWave * leafWeight * gMaterial.leafFlutterAmount * totalWind;
    
    float3 finalLocalPos = displacedPos + flutterOffset;

    // -------------------------------------------------------------------------
    // 3. 影用の行列変換 (Cascade Shadow)
    // -------------------------------------------------------------------------
    float4 worldPos = mul(worldMat, float4(finalLocalPos, 1.0f));
    
    // カスケードシャドウのビュープロジェクション行列を掛けてクリップ空間へ
    output.position = mul(worldPos, gShadowData.cascadeLightViewProj[gCascadeIndex]);
    output.texcoord = input.texcoord;

    return output;
}