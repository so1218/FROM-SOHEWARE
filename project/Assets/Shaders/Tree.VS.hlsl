#include "Object3D.hlsli"
#include "ShaderConstants.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<GlobalEnvironmentData> gEnvironmentData : register(b4);
ConstantBuffer<LeafMaterialData> gMaterial : register(b5);
cbuffer InstanceOffset : register(b9)
{
    uint gBaseInstanceIndex;
};
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
    float3 bitangent : BITANGENT;
    float3 worldPosition : WORLD_POSITION;
    float4 color : COLOR0; // GustMask と 擬似AO をPSに渡すために使用
    float3 instanceTint : COLOR1;
    float lodFade : BLENDWEIGHT;
};

PixelInput main(VertexInput input, uint instanceID : SV_InstanceID)
{
    PixelInput output;
    
    uint actualIndex = instanceID + gBaseInstanceIndex;
    TreeInstanceData instance = gInstanceData[actualIndex];
    
    // input.position は float4 なので .xyz を取得
    float3 origLocalPos = input.position.xyz;
    float4 localPos = float4(origLocalPos, 1.0f);
    
    // ★ ModelRenderer と同じ乗算順序 (localPos * World)
    float4 baseWorldPos = mul(localPos, instance.worldMatrix);
    
    // ★ 修正点2: 平行移動成分は 4 行目 (index 3) から取得
    float3 rootPos = instance.worldMatrix[3].xyz;
    
    // 高さと風のウェイト計算
    float currentHeight = baseWorldPos.y - rootPos.y;
    
    // ウェイト計算（ローカル座標ではなく、ワールドの高さや広がりベースにするのが安全）
    float trunkWeight = pow(saturate(currentHeight / max(gMaterial.treeHeight, 0.1f)), 1.5f);
    float branchWeight = saturate(length(baseWorldPos.xz - rootPos.xz) / max(gMaterial.treeRadius, 0.001f));
    float leafWeight = gMaterial.isLeaf;
    float pseudoAO = lerp(0.3f, 1.0f, saturate(currentHeight / max(gMaterial.treeHeight * 0.5f, 0.1f)));

    // 風の計算
    float2 windDir = normalize(gEnvironmentData.windDirection);
    float windTime = gFrameData.gTime * gEnvironmentData.windSpeed;
    
    float2 windUV = (rootPos.xz * gMaterial.gustScale) - windDir * windTime * 0.05f;
    float gustNoise = gWindMap.SampleLevel(gLinearWrapSampler, windUV, 0).r;
    float gustMask = smoothstep(0.2f, 0.8f, gustNoise);
    float totalWind = gMaterial.baseWindStrength + (gustMask * gMaterial.gustStrength);
    float treePhase = dot(rootPos.xz, float2(0.1f, 0.1f)) + instance.colorVariation.x * 10.0f;
    
    // -------------------------------------------------------------------------
    // ★ ワールド空間でのオフセット計算
    // -------------------------------------------------------------------------
    float globalWave = sin(windTime * 1.0f + treePhase) * 0.5f + 0.5f;
    float3 trunkOffset = float3(windDir.x, 0.0f, windDir.y) * globalWave * trunkWeight * gMaterial.trunkFlexibility * totalWind;
    
    float branchPhase = origLocalPos.x * 0.5f + origLocalPos.y * 0.5f + origLocalPos.z * 0.5f;
    float branchWave = sin(windTime * 2.5f * gEnvironmentData.windTurbulence + treePhase + branchPhase);
    float3 branchDir = normalize(float3(windDir.x, -0.5f, windDir.y));
    float3 branchOffset = branchDir * branchWave * branchWeight * trunkWeight * gMaterial.branchFlexibility * totalWind;
    
    float flutterPhase = dot(origLocalPos, float3(3.0f, 3.0f, 3.0f));
    float flutterWave = sin(windTime * 15.0f + flutterPhase) * cos(windTime * 11.0f + flutterPhase * 0.5f);
    
    // 法線はワールド空間に変換してからバタつきに使用する
    float3 worldNormal = normalize(mul(input.normal, (float3x3) instance.worldMatrix));
    float3 flutterOffset = worldNormal * flutterWave * leafWeight * gMaterial.leafFlutterAmount * totalWind;
    
    // すべてのオフセットをワールド空間で加算
    float3 totalOffset = trunkOffset + branchOffset + flutterOffset;
    
    // ★ Arc Preservation の修正（X, Zの移動量に応じてYを少し下げて長さを維持する簡易計算）
    float offsetLengthXZ = length(totalOffset.xz);
    totalOffset.y -= offsetLengthXZ * currentHeight * 0.1f; // 曲がった分だけ下がる
    
    float3 finalWorldPos = baseWorldPos.xyz + totalOffset;

    // -------------------------------------------------------------------------
    // 法線・接線の計算
    // -------------------------------------------------------------------------
    float3 worldTangent = normalize(mul(input.tangent.xyz, (float3x3) instance.worldMatrix));
    
    // 風の影響で法線を少し傾ける（強すぎると黒くなるのでスケールを下げる）
    float3 tiltedWorldNormal = normalize(worldNormal + totalOffset * 0.1f);
    float3 tiltedWorldTangent = normalize(worldTangent + totalOffset * 0.1f);

    // -------------------------------------------------------------------------
    // 出力
    // -------------------------------------------------------------------------
    output.position = mul(float4(finalWorldPos, 1.0f), gFrameData.viewProjectionMatrix);
    output.worldPosition = finalWorldPos;
    output.texcoord = input.texcoord;
    
    output.normal = tiltedWorldNormal;
    output.tangent = tiltedWorldTangent;
    
    output.bitangent = cross(output.normal, output.tangent);
    
    output.color = float4(gustMask, 0.0f, 0.0f, pseudoAO);
    output.instanceTint = instance.colorVariation.yzw;
    output.lodFade = instance.lodFade;

    return output;
}