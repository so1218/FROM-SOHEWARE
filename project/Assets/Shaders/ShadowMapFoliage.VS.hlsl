#include "Object3D.hlsli"
#include "ShaderConstants.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<LeafMaterialData> gMaterial : register(b5);
ConstantBuffer<ShadowData> gShadowData : register(b8);
ConstantBuffer<CascadeConstant> gCascadeConstant : register(b9);
StructuredBuffer<TreeInstanceData> gInstanceData : register(t10);

Texture2D<float> gWindMap : register(t11);
SamplerState gLinearWrapSampler : register(s2);

struct LeafShadowVSInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD;
    float4 color : COLOR; // R:幹, G:枝, B:葉の風ウェイト
};

struct LeafShadowVSOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

LeafShadowVSOutput LeafShadowVS(LeafShadowVSInput input, uint instanceID : SV_InstanceID)
{
    LeafShadowVSOutput output;
    TreeInstanceData instance = gInstanceData[instanceID];
    
    float4x4 worldMat = instance.worldMatrix;
    float3 origLocalPos = input.position;
    float3 rootPos = float3(worldMat[0][3], worldMat[1][3], worldMat[2][3]);
    
    // 1. 風の強度計算（メインVSと完全に同一の式）
    float2 windDir = normalize(gMaterial.windDir);
    float windTime = gFrameData.gTime * gMaterial.windSpeed;
    
    float2 windUV = (rootPos.xz * gMaterial.gustScale) - windDir * windTime * 0.05f;
    float gustNoise = gWindMap.SampleLevel(gLinearWrapSampler, windUV, 0).r;
    float gustMask = smoothstep(0.2f, 0.8f, gustNoise);
    float totalWind = gMaterial.baseWindStrength + (gustMask * gMaterial.gustStrength);

    float treePhase = dot(rootPos.xz, float2(0.1f, 0.1f)) + instance.colorVariation.x * 10.0f;
    
    // 2. オフセットと曲がり計算（メインVSと完全に同一の式）
    float trunkWave = sin(windTime * 1.2f + treePhase);
    float3 trunkOffset = float3(windDir.x, 0.0f, windDir.y) * trunkWave * input.color.r * gMaterial.trunkFlexibility * totalWind;
    
    float branchWave = sin(windTime * 3.5f + treePhase + origLocalPos.y);
    float3 branchOffset = float3(windDir.x, -0.2f, windDir.y) * branchWave * input.color.g * gMaterial.branchFlexibility * totalWind;
    
    float3 displacedPos = origLocalPos + trunkOffset + branchOffset;
    
    // 長さの維持
    float origLen = length(origLocalPos);
    if (origLen > 0.001f)
    {
        displacedPos = normalize(displacedPos) * origLen;
    }
    
    // 葉のバタつき
    float flutterWave = sin(windTime * 15.0f + origLocalPos.x * 3.0f + origLocalPos.z * 3.0f);
    float3 flutterOffset = input.normal * flutterWave * input.color.b * gMaterial.leafFlutterAmount * totalWind;
    
    float3 finalLocalPos = displacedPos + flutterOffset;

    // 3. ワールド変換 -> ライトのクリップ空間変換
    float4 worldPos = mul(worldMat, float4(finalLocalPos, 1.0f));
    output.position = mul(worldPos, gShadowData.cascadeLightViewProj[gCascadeConstant.cascadeIndex]);
    output.texcoord = input.texcoord;

    return output;
}