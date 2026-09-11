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
    float3 tangent : TANGENT0;
    uint instanceID : SV_InstanceID;
};

struct FoliageVSOutput
{
    float4 position : SV_POSITION;
    float3 worldPosition : WORLD_POSITION;
    float2 texcoord : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float4 color : COLOR0;
    float3 instanceTint : COLOR1;
    float2 velocity : TEXCOORD1;
};

float3 RotateVectorByQuat(float3 v, float4 q)
{
    float3 t = 2.0f * cross(q.xyz, v);
    return v + q.w * t + cross(q.xyz, t);
}

// プロシージャル風変位
float3 CalculateWindDisplacement(float3 worldPos, float windWeight, float3 basePos)
{
    float windLen = length(gEnvironmentData.windDirection);
    float3 windDir = (windLen > kEpsilon)
        ? float3(gEnvironmentData.windDirection.x / windLen, 0.0f, gEnvironmentData.windDirection.y / windLen)
        : float3(0.0f, 0.0f, 1.0f);

    float windSpeed = gEnvironmentData.windSpeed;
    float phase = dot(basePos.xz, float2(0.1f, 0.1f)) + (gEnvironmentData.windTime * windSpeed);
    float bentWeight = pow(windWeight, gMaterial.stiffness);

    float sway = sin(phase) * 0.5f + 0.5f;
    float flutter = sin(phase * gMaterial.flutterSpeed * PI) * gMaterial.flutterScale;

    float gustPhase = dot(basePos.xz, float2(0.05f, 0.05f)) + (gEnvironmentData.windTime * windSpeed * gEnvironmentData.windTurbulence);
    float gust = saturate(sin(gustPhase) * 0.5f + 0.5f);

    return windDir * ((sway + flutter * gust) * bentWeight * gMaterial.windResponse);
}

FoliageVSOutput main(FoliageVSInput input)
{
    FoliageVSOutput output;

    FoliageInstanceData instance = gInstanceData[input.instanceID];

    float3 basePos = instance.posAndScale.xyz;
    float scale = instance.posAndScale.w;
    float4 quat = instance.rotationQuat;
    float3 localPos = input.position.xyz;

    float windWeight = saturate(localPos.y / max(gMaterial.plantHeight, kEpsilon));
    localPos *= scale;

    float3 rotatedPos = RotateVectorByQuat(localPos, quat);
    float3 worldPos = basePos + rotatedPos;

    // 風変位
    float3 windDisp = CalculateWindDisplacement(worldPos, windWeight, basePos);

   // インタラクション変位
    float3 pushDisp = float3(0.0f, 0.0f, 0.0f);
    float2 interactUV = (basePos.xz - gInteractionData.centerWorldPos) / gInteractionData.worldSize + 0.5f;

    // マップ境界の端10%の領域で滑らかにフェードアウトする係数を作成
    float2 edgeFade = smoothstep(0.0f, 0.1f, interactUV) * smoothstep(1.0f, 0.9f, interactUV);
    float edgeMultiplier = edgeFade.x * edgeFade.y;

    if (edgeMultiplier > kEpsilon && all(interactUV >= 0.0f) && all(interactUV <= 1.0f))
    {
        float4 interactData = gInteractionMap.SampleLevel(gLinearClampSampler, interactUV, 0);

        float2 pushDirXZ = (interactData.rg * 2.0f) - 1.0f;
        float dirLen = length(pushDirXZ);
        pushDirXZ = (dirLen > kEpsilon) ? (pushDirXZ / dirLen) : float2(0.0f, 0.0f);

        float instantPower = interactData.b;
        float rawTrailPower = interactData.a * gMaterial.trailFlattenWeight;

        // 減衰カーブ補正
        float combinedPower = max(instantPower, rawTrailPower);
        float smoothPower = smoothstep(0.0f, 1.0f, combinedPower);
        smoothPower = pow(smoothPower, max(gMaterial.recoverySpeed, 0.1f));

        // エッジ係数を掛けて境界付近で滑らかに元の状態（0）へ戻す
        smoothPower *= edgeMultiplier;

        // 倒れ込み変位
        float3 flattenDir = normalize(float3(pushDirXZ.x, -gMaterial.flattenFactor, pushDirXZ.y));
        float3 flattenDisp = flattenDir * smoothPower * gMaterial.interactStrength * windWeight;

        // 復元時の減衰バネ振動
        float springPhase = (gEnvironmentData.windTime * 6.0f) + dot(basePos.xz, float2(12.9898f, 78.233f));
        float springEnvelope = 4.0f * smoothPower * (1.0f - smoothPower);
        float wave = sin(springPhase);

        float3 springDir = float3(-pushDirXZ.x * wave, abs(wave) * 0.4f, -pushDirXZ.y * wave);
        float3 springDisp = springDir * springEnvelope * gMaterial.springElasticity * gMaterial.interactStrength * windWeight;

        pushDisp = flattenDisp + springDisp;

        // 倒れ込み中の風揺れ減衰 (フェードアウト時は自然な風揺れに戻る)
        windDisp *= saturate(1.0f - smoothPower * 1.2f);
    }

    // 最終座標計算と変形歪み（座屈）の補正
    worldPos += (windDisp + pushDisp);

    float currentLen = length(worldPos - basePos);
    float originalLen = length(rotatedPos);
    if (currentLen > kEpsilon)
    {
        worldPos = basePos + (worldPos - basePos) * (originalLen / currentLen);
    }

    output.worldPosition = worldPos;

    // 法線・接線の回転
    float3 localNormal = normalize(input.normal);
    float3 localTangent = normalize(input.tangent);
    output.normal = normalize(RotateVectorByQuat(localNormal, quat));
    output.tangent = normalize(RotateVectorByQuat(localTangent, quat));

    // 座標変換 & TAA用MotionVector
    float4 clipPos = mul(float4(worldPos, 1.0f), gFrameData.viewProjectionMatrix);
    float4 prevClipPos = mul(float4(basePos + rotatedPos, 1.0f), gFrameData.prevViewProj);

    output.position = clipPos;
    output.velocity = (clipPos.xy / clipPos.w - prevClipPos.xy / prevClipPos.w) * float2(0.5f, -0.5f);

    output.texcoord = input.texcoord;
    output.color = float4(windWeight, 0.0f, 0.0f, 0.0f);
    output.instanceTint = instance.colorVariation;

    return output;
}