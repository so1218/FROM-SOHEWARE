#include "Object3D.hlsli"
#include "ShaderConstants.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<LeafMaterialData> gMaterial : register(b5);
StructuredBuffer<TreeInstanceData> gInstanceData : register(t10);

// 風マップとテクスチャ群
Texture2D<float> gWindMap : register(t11);

SamplerState gLinearWrapSampler : register(s2);

struct VertexInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
    float2 texcoord : TEXCOORD;
    // 頂点カラーに風のウェイトを仕込む
    // R: 幹の揺れやすさ, G: 枝の揺れやすさ, B: 葉の細かなバタつき, A: 頂点AO
    float4 color : COLOR;
};

struct PixelInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
    float3 worldPosition : WORLD_POSITION;
    float4 color : COLOR0;
    float3 instanceTint : COLOR1;
    float lodFade : BLENDWEIGHT;
    bool isFrontFace : SV_IsFrontFace;
};

PixelInput main(VertexInput input, uint instanceID : SV_InstanceID)
{
    PixelInput output;
    TreeInstanceData instance = gInstanceData[instanceID];
    
    float4x4 worldMat = instance.worldMatrix;
    float3 localPos = input.position;
    float3 rootPos = float3(worldMat[0][3], worldMat[1][3], worldMat[2][3]);
    
    // 風の計算
    float2 windDir = normalize(gMaterial.windDir);
    float windTime = gFrameData.gTime * gMaterial.windSpeed;
    
    float2 windUV = (rootPos.xz * gMaterial.gustScale) - windDir * windTime * 0.05f;
    float gustNoise = gWindMap.SampleLevel(gLinearWrapSampler, windUV, 0).r;
    float gustMask = smoothstep(0.2f, 0.8f, gustNoise);
    float totalWind = gMaterial.baseWindStrength + (gustMask * gMaterial.gustStrength);

    float treePhase = dot(rootPos.xz, float2(0.1f, 0.1f)) + instance.colorVariation.x * 10.0f;
    
    float trunkWave = sin(windTime * 1.2f + treePhase);
    float3 trunkOffset = float3(windDir.x, 0.0f, windDir.y) * trunkWave * input.color.r * gMaterial.trunkFlexibility * totalWind;
    
    float branchWave = sin(windTime * 3.5f + treePhase + localPos.y);
    float3 branchOffset = float3(windDir.x, -0.2f, windDir.y) * branchWave * input.color.g * gMaterial.branchFlexibility * totalWind;
    
    float flutterWave = sin(windTime * 15.0f + localPos.x * 3.0f + localPos.z * 3.0f);
    float3 flutterOffset = input.normal * flutterWave * input.color.b * gMaterial.leafFlutterAmount * totalWind;

    localPos += trunkOffset + branchOffset + flutterOffset;
    
    // 座標変換
    float4 worldPos = mul(worldMat, float4(localPos, 1.0f));
    output.position = mul(gFrameData.viewProjectionMatrix, worldPos);
    output.worldPosition = worldPos.xyz;
    output.texcoord = input.texcoord;
    
    output.normal = normalize(mul((float3x3) worldMat, input.normal));
    output.tangent = normalize(mul((float3x3) worldMat, input.tangent.xyz));
    output.bitangent = cross(output.normal, output.tangent) * input.tangent.w;
    
    output.color = float4(input.color.rgb, gustMask);
    
    // 個体ごとの色ブレとフェード値をPSへ渡す
    output.instanceTint = instance.colorVariation.yzw;
    output.lodFade = instance.lodFade;

    return output;
}