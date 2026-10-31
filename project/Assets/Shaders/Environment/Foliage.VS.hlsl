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
};

float3 RotateVectorByQuat(float3 v, float4 q)
{
    float3 t = 2.0f * cross(q.xyz, v);
    return v + q.w * t + cross(q.xyz, t);
}

// 風で横にずらす量
float3 CalculateWindOffset(float3 rootPosition, float heightRatio)
{
    float windLength = length(gEnvironmentData.windDirection);
    float2 windDirection = (windLength > kEpsilon) ? gEnvironmentData.windDirection / windLength : float2(0.0f, 1.0f);

    // 場所ごとに揺れのタイミングをずらす
    float phase = dot(rootPosition.xz, windDirection * 0.15f);
    float time = gEnvironmentData.windTime;

    float mainSway = sin(time + phase);
    float detailSway = sin(time * gMaterial.flutterSpeed + phase * 3.0f) * gMaterial.flutterScale;

    // 根元は動かさず、上ほど大きく揺らす
    float sway = (mainSway + detailSway) * heightRatio * heightRatio * gMaterial.windResponse * gEnvironmentData.windSpeed;
    return float3(windDirection.x * sway, 0.0f, windDirection.y * sway);
}

FoliageVSOutput main(FoliageVSInput input)
{
    FoliageInstanceData instance = gInstanceData[input.instanceID];
    float3 rootPosition = instance.posAndScale.xyz;
    float scale = instance.posAndScale.w;

    // 根元 0 〜 植物の高さ 1
    float heightRatio = saturate(input.position.y / max(gMaterial.plantHeight, kEpsilon));

    float3 rotatedPosition = RotateVectorByQuat(input.position.xyz * scale, instance.rotationQuat);
    float3 offset = CalculateWindOffset(rootPosition, heightRatio);

    // プレイヤーなどに押されて倒れる。インタラクションの範囲の端では効果を弱める
    float2 interactionUV = (rootPosition.xz - gInteractionData.centerWorldPos) / gInteractionData.worldSize + 0.5f;
    float2 edgeFade = smoothstep(0.0f, 0.1f, interactionUV) * smoothstep(1.0f, 0.9f, interactionUV);
    float edgeWeight = edgeFade.x * edgeFade.y;

    if (edgeWeight > kEpsilon)
    {
        float4 interaction = gInteractionMap.SampleLevel(gLinearClampSampler, interactionUV, 0);

        float2 pushDirection = interaction.rg * 2.0f - 1.0f;
        float pushLength = length(pushDirection);
        pushDirection = (pushLength > kEpsilon) ? pushDirection / pushLength : float2(0.0f, 0.0f);

        // b = 今押されている強さ, a = 通った跡
        float pushPower = smoothstep(0.0f, 1.0f, max(interaction.b, interaction.a * gMaterial.trailFlattenWeight)) * edgeWeight;

        // 倒れている間は風で揺れにくくする
        offset *= saturate(1.0f - pushPower * 1.2f);
        offset.xz += pushDirection * pushPower * gMaterial.interactStrength * heightRatio;
    }

    // 横にずれた分だけ低くして、長さを保つ (伸びて見えないように)
    float3 bentPosition = rotatedPosition + offset;
    float bentHeight = sqrt(max(0.0f, dot(rotatedPosition, rotatedPosition) - dot(bentPosition.xz, bentPosition.xz)));
    bentPosition.y = (rotatedPosition.y < 0.0f) ? -bentHeight : bentHeight;

    float3 worldPosition = rootPosition + bentPosition;

    FoliageVSOutput output;
    output.position = mul(float4(worldPosition, 1.0f), gFrameData.viewProjectionMatrix);
    output.worldPosition = worldPosition;
    output.texcoord = input.texcoord;
    output.normal = normalize(RotateVectorByQuat(normalize(input.normal), instance.rotationQuat));
    return output;
}
