#include "Common/ShaderConstants.hlsli"
#include "Common/MathUtils.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<GlobalEnvironmentData> gEnvironmentData : register(b4);
ConstantBuffer<FoliageMaterialData> gMaterial : register(b5);
ConstantBuffer<InteractionConstants> gInteractionData : register(b6);
StructuredBuffer<FoliageInstanceData> gInstanceData : register(t10);

Texture2D<float4> gInteractionMap : register(t3);
SamplerState gLinearClampSampler : register(s0);

struct FoliageVSInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
    uint instanceID : SV_InstanceID;
};

struct FoliageVSOutput
{
    float4 position : SV_POSITION;
    float3 worldPosition : WORLD_POSITION;
    float2 texcoord : TEXCOORD;
    float3 normal : NORMAL;
    float4 color : COLOR0;
    float3 instanceTint : COLOR1;
};

float3 RotateVectorByQuat(float3 v, float4 q)
{
    float3 t = 2.0f * cross(q.xyz, v);
    return v + q.w * t + cross(q.xyz, t);
}

// プロシージャル風変位
float3 CalculateWindDisplacement(float3 basePos, float windWeight)
{
    // 草のしなり：高さの2乗（Y^2）に比例させるのが物理的に自然でpow不要（高速）
    float bendFactor = windWeight * windWeight * gMaterial.windResponse;

    float windLen = length(gEnvironmentData.windDirection);
    float2 windDir = (windLen > kEpsilon)
        ? (gEnvironmentData.windDirection / windLen)
        : float2(0.0f, 1.0f);

    // 空間位相（フィールドを風の波が伝わる表現）
    float spatialPhase = dot(basePos.xz, windDir * 0.15f);
    float time = gEnvironmentData.windTime * gEnvironmentData.windSpeed;

    // 主揺れ (Sway) + 細かい揺れ (Flutter)
    float mainSway = sin(time + spatialPhase);
    float detailSway = sin(time * gMaterial.flutterSpeed + spatialPhase * 3.0f) * gMaterial.flutterScale;

    float totalSway = (mainSway + detailSway) * bendFactor;

    return float3(windDir.x * totalSway, 0.0f, windDir.y * totalSway);
}

FoliageVSOutput main(FoliageVSInput input)
{
    FoliageVSOutput output;

    FoliageInstanceData instance = gInstanceData[input.instanceID];

    float3 basePos = instance.posAndScale.xyz;
    float scale = instance.posAndScale.w;
    float4 quat = instance.rotationQuat;
    float3 localPos = input.position.xyz;

    // 頂点の高さ比率
    float windWeight = saturate(localPos.y / max(gMaterial.plantHeight, kEpsilon));

    // ローカル変換（スケール ＆ 回転）
    localPos *= scale;
    float3 rotatedPos = RotateVectorByQuat(localPos, quat);

    // 風変位の計算
    float3 totalDisp = CalculateWindDisplacement(basePos, windWeight);

    // インタラクション
    float2 interactUV = (basePos.xz - gInteractionData.centerWorldPos) / gInteractionData.worldSize + 0.5f;
    float2 edgeFade = smoothstep(0.0f, 0.1f, interactUV) * smoothstep(1.0f, 0.9f, interactUV);
    float edgeMultiplier = edgeFade.x * edgeFade.y;

    if (edgeMultiplier > kEpsilon && all(interactUV >= 0.0f) && all(interactUV <= 1.0f))
    {
        float4 interactData = gInteractionMap.SampleLevel(gLinearClampSampler, interactUV, 0);

        // 押し出し方向 
        float2 pushDirXZ = interactData.rg * 2.0f - 1.0f;
        float dirLen = length(pushDirXZ);
        pushDirXZ = (dirLen > kEpsilon) ? (pushDirXZ / dirLen) : float2(0.0f, 0.0f);

        // 押し出し強度の合成
        float pushPower = max(interactData.b, interactData.a * gMaterial.trailFlattenWeight);
        float smoothPower = smoothstep(0.0f, 1.0f, pushPower) * edgeMultiplier;

        // 水平方向への押し倒し量
        float2 pushOffset = pushDirXZ * (smoothPower * gMaterial.interactStrength * windWeight);

        // 倒れ込み中は風の影響を弱める
        totalDisp *= saturate(1.0f - smoothPower * 1.2f);
        totalDisp.xz += pushOffset;
    }
    
    // 水平方向に傾いた分だけ、三平方の定理で正確にY座標を下げる
    float originalLen = length(rotatedPos);
    float3 targetLocalXZ = rotatedPos + totalDisp;
    float distXZSq = dot(targetLocalXZ.xz, targetLocalXZ.xz);

    float3 finalLocalPos;
    finalLocalPos.xz = targetLocalXZ.xz;
    
    // 元の長さを保つように高さYを補正
    finalLocalPos.y = sqrt(max(0.0f, originalLen * originalLen - distXZSq));
    if (rotatedPos.y < 0.0f)
        finalLocalPos.y = -finalLocalPos.y; 

    float3 worldPos = basePos + finalLocalPos;
    output.worldPosition = worldPos;

    // 法線の回転
    float3 localNormal = normalize(input.normal);
    output.normal = normalize(RotateVectorByQuat(localNormal, quat));

    // 座標変換
    output.position = mul(float4(worldPos, 1.0f), gFrameData.viewProjectionMatrix);

    output.texcoord = input.texcoord;
    output.color = float4(windWeight, 0.0f, 0.0f, 0.0f);
    output.instanceTint = instance.colorVariation;

    return output;
}