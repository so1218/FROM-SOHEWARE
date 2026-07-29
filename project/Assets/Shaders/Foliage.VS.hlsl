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
};

PixelInput main(VertexInput input, uint instanceID : SV_InstanceID)
{
    PixelInput output;
    TreeInstanceData instance = gInstanceData[instanceID];
    
    float4x4 worldMat = instance.worldMatrix;
    float3 origLocalPos = input.position; // 元のローカル座標を保持
    float3 rootPos = float3(worldMat[0][3], worldMat[1][3], worldMat[2][3]);
    
    // -------------------------------------------------------------------------
    // 1. 風の強度とウェイト計算
    // -------------------------------------------------------------------------
    float2 windDir = normalize(gMaterial.windDir);
    float windTime = gFrameData.gTime * gMaterial.windSpeed;
    
    float2 windUV = (rootPos.xz * gMaterial.gustScale) - windDir * windTime * 0.05f;
    float gustNoise = gWindMap.SampleLevel(gLinearWrapSampler, windUV, 0).r;
    float gustMask = smoothstep(0.2f, 0.8f, gustNoise);
    float totalWind = gMaterial.baseWindStrength + (gustMask * gMaterial.gustStrength);

    float treePhase = dot(rootPos.xz, float2(0.1f, 0.1f)) + instance.colorVariation.x * 10.0f;
    
    // -------------------------------------------------------------------------
    // 2. オフセット計算とストレッチ防止 (Arc Preservation)
    // -------------------------------------------------------------------------
    float trunkWave = sin(windTime * 1.2f + treePhase);
    float3 trunkOffset = float3(windDir.x, 0.0f, windDir.y) * trunkWave * input.color.r * gMaterial.trunkFlexibility * totalWind;
    
    float branchWave = sin(windTime * 3.5f + treePhase + origLocalPos.y);
    float3 branchOffset = float3(windDir.x, -0.2f, windDir.y) * branchWave * input.color.g * gMaterial.branchFlexibility * totalWind;
    
    // まず幹と枝の大きな揺れを適用
    float3 displacedPos = origLocalPos + trunkOffset + branchOffset;
    
    // 【追加】長さの維持（根元からの距離を保つことで、伸びるのではなく「曲がる」ようにする）
    float origLen = length(origLocalPos);
    if (origLen > 0.001f)
    {
        displacedPos = normalize(displacedPos) * origLen;
    }
    
    // 葉のバタつきは局所的な変形なので長さ補正の後に加算
    float flutterWave = sin(windTime * 15.0f + origLocalPos.x * 3.0f + origLocalPos.z * 3.0f);
    float3 flutterOffset = input.normal * flutterWave * input.color.b * gMaterial.leafFlutterAmount * totalWind;
    
    float3 finalLocalPos = displacedPos + flutterOffset;

    // -------------------------------------------------------------------------
    // 3. 法線・接線の回転補正 (Normal Tilt)
    // -------------------------------------------------------------------------
    // 【追加】頂点がどれだけ移動したか（デルタ）を算出し、その方向へ法線を少し傾ける
    float3 posDelta = finalLocalPos - origLocalPos;
    
    // 行列を使わない軽量なフェイク回転。0.5f は傾き具合の調整用係数
    float3 tiltedLocalNormal = normalize(input.normal + posDelta * 0.5f);
    float3 tiltedLocalTangent = normalize(input.tangent.xyz + posDelta * 0.5f);

    // -------------------------------------------------------------------------
    // 4. ワールド変換と出力
    // -------------------------------------------------------------------------
    float4 worldPos = mul(worldMat, float4(finalLocalPos, 1.0f));
    output.position = mul(gFrameData.viewProjectionMatrix, worldPos);
    output.worldPosition = worldPos.xyz;
    output.texcoord = input.texcoord;
    
    // 傾けた法線をワールド空間へ変換
    output.normal = normalize(mul((float3x3) worldMat, tiltedLocalNormal));
    output.tangent = normalize(mul((float3x3) worldMat, tiltedLocalTangent));
    output.bitangent = cross(output.normal, output.tangent) * input.tangent.w;
    
    output.color = float4(input.color.rgb, gustMask);
    output.instanceTint = instance.colorVariation.yzw;
    output.lodFade = instance.lodFade;

    return output;
}