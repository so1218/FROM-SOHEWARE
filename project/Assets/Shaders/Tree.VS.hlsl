#include "Object3D.hlsli"
#include "ShaderConstants.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<LeafMaterialData> gMaterial : register(b5);
StructuredBuffer<TreeInstanceData> gInstanceData : register(t10);

Texture2D<float> gWindMap : register(t11);
SamplerState gLinearWrapSampler : register(s2);

struct VertexInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
    float2 texcoord : TEXCOORD;
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
    TreeInstanceData instance = gInstanceData[instanceID];
    
    float4x4 worldMat = instance.worldMatrix;
    float3 origLocalPos = input.position;
    float3 rootPos = float3(worldMat[0][3], worldMat[1][3], worldMat[2][3]);
    
    // -------------------------------------------------------------------------
    // 【ハック】ローカル座標から風ウェイトと擬似AOを生成
    // -------------------------------------------------------------------------
    // 幹のウェイト: 根元は0、上に行くほど1。pow(x, 1.5)で根元を硬く、先端を柔らかくしならせる。
    float trunkWeight = pow(saturate(origLocalPos.y / gMaterial.treeHeight), 1.5f);
    
    // 枝のウェイト: 幹の中心(X=0, Z=0)から離れるほど1。
    float branchWeight = saturate(length(origLocalPos.xz) / max(gMaterial.treeRadius, 0.001f));
    
    // 葉のウェイト: 定数バッファのフラグを使用
    float leafWeight = gMaterial.isLeaf;

    // 擬似的な頂点AO (根元ほど暗く、幹の内側ほど暗くする)
    float pseudoAO = lerp(0.3f, 1.0f, saturate(origLocalPos.y / (gMaterial.treeHeight * 0.5f)));

    // -------------------------------------------------------------------------
    // 1. 風の全体的な強度とマップサンプリング
    // -------------------------------------------------------------------------
    float2 windDir = normalize(gMaterial.windDir);
    float windTime = gFrameData.gTime * gMaterial.windSpeed;
    
    // マップからGust(突風)を取得。ワールド空間で流すことで、森全体を波が走るようにする
    float2 windUV = (rootPos.xz * gMaterial.gustScale) - windDir * windTime * 0.05f;
    float gustNoise = gWindMap.SampleLevel(gLinearWrapSampler, windUV, 0).r;
    float gustMask = smoothstep(0.2f, 0.8f, gustNoise);
    float totalWind = gMaterial.baseWindStrength + (gustMask * gMaterial.gustStrength);

    // 木ごとの固有の位相 (同期して揺れるのを防ぐ)
    float treePhase = dot(rootPos.xz, float2(0.1f, 0.1f)) + instance.colorVariation.x * 10.0f;
    
    // -------------------------------------------------------------------------
    // 2. AAA級 オフセット計算 (Global -> Branch -> Flutter)
    // -------------------------------------------------------------------------
    
    // [Layer 1: Global Bending] 幹全体のしなり
    // 一定方向への押し込み ＋ ゆっくりとした揺り返し
    float globalWave = sin(windTime * 1.0f + treePhase) * 0.5f + 0.5f; // 0.0 ~ 1.0
    float3 trunkOffset = float3(windDir.x, 0.0f, windDir.y) * globalWave * trunkWeight * gMaterial.trunkFlexibility * totalWind;
    
    // [Layer 2: Branch Bending] 枝の独立した揺れ
    // ローカル座標を位相に混ぜることで、枝ごとに揺れるタイミングをずらす（Turbulence）
    float branchPhase = origLocalPos.x * 0.5f + origLocalPos.y * 0.5f + origLocalPos.z * 0.5f;
    float branchWave = sin(windTime * 2.5f * gMaterial.windTurbulence + treePhase + branchPhase);
    
    // 枝は風に押されるだけでなく、上下（Y軸）にもバウンドするようにする
    float3 branchDir = normalize(float3(windDir.x, -0.5f, windDir.y));
    float3 branchOffset = branchDir * branchWave * branchWeight * trunkWeight * gMaterial.branchFlexibility * totalWind;
    
    // 幹と枝の揺れを合成
    float3 displacedPos = origLocalPos + trunkOffset + branchOffset;
    
    // [Arc Preservation] 長さの維持（伸びるのを防ぎ、曲がるようにする）
    float origLen = length(origLocalPos);
    if (origLen > 0.001f)
    {
        displacedPos = normalize(displacedPos) * origLen;
    }
    
    // [Layer 3: Leaf Flutter] 葉っぱの高速なバタつき
    // マテリアルが「葉」の場合のみ適用。細かいノイズ的な動き。
    float flutterPhase = dot(origLocalPos, float3(3.0f, 3.0f, 3.0f));
    float flutterWave = sin(windTime * 15.0f + flutterPhase) * cos(windTime * 11.0f + flutterPhase * 0.5f);
    
    // 葉は法線方向に細かく震える
    float3 flutterOffset = input.normal * flutterWave * leafWeight * gMaterial.leafFlutterAmount * totalWind;
    
    float3 finalLocalPos = displacedPos + flutterOffset;

    // -------------------------------------------------------------------------
    // 3. 法線・接線の回転補正 (Normal Tilt)
    // -------------------------------------------------------------------------
    float3 posDelta = finalLocalPos - origLocalPos;
    
    // 風による変位量から、法線を少し「風下」へ傾ける。これでライティングが動的に変化し、揺れが強調される。
    float3 tiltedLocalNormal = normalize(input.normal + posDelta * 0.8f);
    float3 tiltedLocalTangent = normalize(input.tangent.xyz + posDelta * 0.8f);

    // -------------------------------------------------------------------------
    // 4. ワールド変換と出力
    // -------------------------------------------------------------------------
    float4 worldPos = mul(worldMat, float4(finalLocalPos, 1.0f));
    output.position = mul(gFrameData.viewProjectionMatrix, worldPos);
    output.worldPosition = worldPos.xyz;
    output.texcoord = input.texcoord;
    
    output.normal = normalize(mul((float3x3) worldMat, tiltedLocalNormal));
    output.tangent = normalize(mul((float3x3) worldMat, tiltedLocalTangent));
    output.bitangent = cross(output.normal, output.tangent) * input.tangent.w;
    
    // ピクセルシェーダーに GustMask と 擬似AO を渡すために color を再利用
    // r: gustMask (Specularの強調などに使用)
    // a: pseudoAO (頂点AOの代わり)
    output.color = float4(gustMask, 0.0f, 0.0f, pseudoAO);
    output.instanceTint = instance.colorVariation.yzw;
    output.lodFade = instance.lodFade;

    return output;
}