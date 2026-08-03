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
    float3 bitangent : BITANGENT;
    float3 worldPosition : WORLD_POSITION;
    float4 color : COLOR0; // GustMask と 擬似AO をPSに渡すために使用
    float3 instanceTint : COLOR1;
    float lodFade : BLENDWEIGHT;
};

float3 RotateAboutAxis(float3 pos, float3 axis, float angle)
{
    float s = sin(angle);
    float c = cos(angle);
    return pos * c + cross(axis, pos) * s + axis * dot(axis, pos) * (1.0f - c);
}

PixelInput main(VertexInput input, uint instanceID : SV_InstanceID)
{
    PixelInput output;
    
    uint actualIndex = instanceID + gTreeInstanceOffset.baseInstanceIndex;
    TreeInstanceData instance = gInstanceData[actualIndex];
    
    float3 origLocalPos = input.position.xyz;
    float4 localPos = float4(origLocalPos, 1.0f);
    
    float4 baseWorldPos = mul(localPos, instance.worldMatrix);
    float3 rootPos = instance.worldMatrix[3].xyz; // 木の根元座標
    
    // 高さと高さ比率（0.0～1.0）
    float currentHeight = max(0.0f, baseWorldPos.y - rootPos.y);
    float heightRatio = saturate(currentHeight / max(gMaterial.treeHeight, 0.1f));
    
    // 葉フラグ (0.0 = 幹, 1.0 = 葉)
    float isLeaf = (float) gTreeInstanceOffset.isLeaf;

    // 擬似AO
    float pseudoAO = lerp(0.3f, 1.0f, saturate(heightRatio * 2.0f));

    // 風の基本計算
    float2 windDir = normalize(gEnvironmentData.windDirection);
    float windTime = gFrameData.gTime * gEnvironmentData.windSpeed;
    
    // ノイズマップから陣風を取得
    float2 windUV = (rootPos.xz * gMaterial.gustScale) - windDir * windTime * 0.05f;
    float gustNoise = gWindMap.SampleLevel(gLinearWrapSampler, windUV, 0).r;
    float gustMask = smoothstep(0.2f, 0.8f, gustNoise);
    float totalWind = gMaterial.baseWindStrength + (gustMask * gMaterial.gustStrength);
    
    // 木ごとの位相
    float treePhase = dot(rootPos.xz, float2(0.13f, 0.17f)) + instance.colorVariation.x * 12.34f;

    // =========================================================================
    // ★ 1次風: 幹の「ピボット回転しなり」（Trunk Bending via Pivot Rotation）
    // =========================================================================
    // 風の進行方向に対して垂直な「回転軸」を算出
    float3 rotAxis = normalize(float3(-windDir.y, 0.0f, windDir.x));
    
    // 根元から上に行くほど大きく曲がる角度（ラジアン）
    // 2乗カーブにより根元付近はほぼ固定され、先端に行くほど綺麗にしなる
    float trunkWeight = heightRatio * heightRatio;
    
    float mainSway = sin(windTime * 1.0f + treePhase) * 0.3f + 0.7f; // 主風
    float subSway = sin(windTime * 1.8f + treePhase * 1.5f) * 0.2f; // 揺れ戻り
    
    // 曲げ角度 (最大約15度～20度程度に制限)
    float bendAngle = (mainSway + subSway) * trunkWeight * gMaterial.trunkFlexibility * totalWind * 0.15f;
    
    // 根元からの相対ベクトルを算出
    float3 relWorldPos = baseWorldPos.xyz - rootPos;
    
    // 【最重要】位置だけでなく、法線・接線も同じ軸で回転させることでライティングの破綻を防ぐ
    float3 bentRelPos = RotateAboutAxis(relWorldPos, rotAxis, bendAngle);
    float3 worldNormal = normalize(mul(input.normal, (float3x3) instance.worldMatrix));
    float3 worldTangent = normalize(mul(input.tangent, (float3x3) instance.worldMatrix));
    
    worldNormal = RotateAboutAxis(worldNormal, rotAxis, bendAngle);
    worldTangent = RotateAboutAxis(worldTangent, rotAxis, bendAngle);

    // =========================================================================
    // ★ 2次風: 枝のうねり（Branch Oscillation） - 葉メッシュのみ
    // =========================================================================
    float branchDist = length(origLocalPos.xz);
    float branchWeight = saturate(branchDist / max(gMaterial.treeRadius, 0.001f));
    
    float branchPhase = dot(origLocalPos, float3(0.5f, 0.8f, 0.3f)) + treePhase;
    float branchWave = sin(windTime * 2.5f * gEnvironmentData.windTurbulence + branchPhase);
    float3 branchDir = normalize(float3(windDir.x, -0.2f, windDir.y));
    
    float3 branchOffset = branchDir
                        * branchWave
                        * branchWeight
                        * trunkWeight
                        * gMaterial.branchFlexibility 
                        * totalWind
                        * isLeaf;

    // =========================================================================
    // ★ 3次風: 葉のチラつき（Leaf Flutter / Rustle） - 葉メッシュのみ
    // =========================================================================
    float flutterPhase = dot(origLocalPos, float3(3.5f, 4.2f, 2.8f)) + treePhase;
    float flutterWave = sin(windTime * 14.0f + flutterPhase) * cos(windTime * 9.0f + flutterPhase * 0.5f);
    
    float3 flutterOffset = worldNormal
                         * flutterWave
                         * gMaterial.leafFlutterAmount 
                         * totalWind
                         * isLeaf;

    // =========================================================================
    // 最終位置の合成 (幹の回転位置 + 枝の揺れ + 葉のバタつき)
    // 幹と葉で「幹の回転位置 (rootPos + bentRelPos)」を完全に共有するため、一切分解しません！
    // =========================================================================
    float3 finalWorldPos = rootPos + bentRelPos + branchOffset + flutterOffset;

    // -------------------------------------------------------------------------
    // 出力
    // -------------------------------------------------------------------------
    output.position = mul(float4(finalWorldPos, 1.0f), gFrameData.viewProjectionMatrix);
    output.worldPosition = finalWorldPos;
    output.texcoord = input.texcoord;
    
    output.normal = worldNormal;
    output.tangent = worldTangent;
    output.bitangent = cross(output.normal, output.tangent);
    
    output.color = float4(gustMask, 0.0f, 0.0f, pseudoAO);
    output.instanceTint = instance.colorVariation.yzw;
    output.lodFade = instance.lodFade;

    return output;
}