#include "Common/Object3D.hlsli"
#include "Common/ShaderConstants.hlsli"
#include "Common/MathUtils.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<GrassMaterialData> gMaterial : register(b5);
ConstantBuffer<GrassCullingData> gGrassCullingData : register(b6);
ConstantBuffer<GlobalEnvironmentData> gEnvironmentData : register(b7);
ConstantBuffer<InteractionConstants> gInteractionData : register(b8);
StructuredBuffer<GrassInstanceData> gInstanceData : register(t10);

Texture2D<float> gWindMap : register(t11);
Texture2D<float4> gInteractionMap : register(t12);

SamplerState gLinearWrapSampler : register(s2);
SamplerState gLinearClampSampler : register(s3);

// TriangleStrip で 8 頂点 = 3 段
static const uint kVerticesPerBlade = 8;

// 幅方向に法線を外へ傾ける量。1枚の板でも丸みがあるように見せる
static const float kNormalRounding = 0.5f;

// 真横を向いた草と、カメラを向いた草の混ぜ具合
static const float kFaceCameraBlend = 0.6f;

float3 EvaluateCubicBezier(float3 p0, float3 p1, float3 p2, float3 p3, float t)
{
    float u = 1.0f - t;
    return u * u * u * p0 + (3.0f * u * u * t) * p1 + (3.0f * u * t * t) * p2 + t * t * t * p3;
}

float3 EvaluateCubicBezierTangent(float3 p0, float3 p1, float3 p2, float3 p3, float t)
{
    float u = 1.0f - t;
    float3 tangent = -3.0f * u * u * p0 +
                     (3.0f * u * u - 6.0f * u * t) * p1 +
                     (6.0f * u * t - 3.0f * t * t) * p2 +
                     3.0f * t * t * p3;
    return normalize(tangent);
}

struct GrassPSInput
{
    float4 position : SV_POSITION;
    float heightRatio : TEXCOORD; // 0:根元 ~ 1:先端
    float3 normal : NORMAL;
    float3 worldPosition : WORLD_POSITION;
};

GrassPSInput main(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
    GrassPSInput output;

    GrassInstanceData instance = gInstanceData[instanceID];
    float3 rootPosition = instance.posAndHeight.xyz;
    float grassHeight = instance.posAndHeight.w;
    float rotationY = instance.rotationAndWidth.x;
    float grassWidth = instance.rotationAndWidth.y;

    float distanceToCamera = distance(rootPosition, gFrameData.cameraWorldPosition);

    // 遠くでは間引いた分だけ幅を広げて、草原が薄く見えないようにする
    float thinRatio = saturate((distanceToCamera - gGrassCullingData.thinStartDistance) /
                               max(gGrassCullingData.maxDrawDistance - gGrassCullingData.thinStartDistance, kEpsilon));
    grassWidth *= lerp(1.0f, gGrassCullingData.maxWidthMultiplier, thinRatio);

    float sinRotation, cosRotation;
    sincos(rotationY, sinRotation, cosRotation);
    float3 randomRight = float3(cosRotation, 0.0f, -sinRotation);
    float3 randomForward = float3(sinRotation, 0.0f, cosRotation);

    // 少しカメラの方を向かせて、真横から見た草が線にならないようにする。真上から見ると向きが決まらないのでそのまま
    float2 toCameraXZ = gFrameData.cameraWorldPosition.xz - rootPosition.xz;
    float3 bladeRight = randomRight;
    if (dot(toCameraXZ, toCameraXZ) > kEpsilon)
    {
        float3 toCamera = normalize(float3(toCameraXZ.x, 0.0f, toCameraXZ.y));
        float3 faceCameraRight = cross(toCamera, float3(0.0f, 1.0f, 0.0f));
        bladeRight = normalize(lerp(randomRight, faceCameraRight, kFaceCameraBlend));
    }

    uint vertexIndex = vertexID % kVerticesPerBlade;
    float heightRatio = (vertexIndex / 2) / (float) (kVerticesPerBlade / 2 - 1);

    // 遠くの草は段数を減らす。余った頂点は先端に重ねる
    if (distanceToCamera > gGrassCullingData.lodDistance2)
    {
        if (vertexIndex >= 2)
            heightRatio = 1.0f;
    }
    else if (distanceToCamera > gGrassCullingData.lodDistance1)
    {
        if (vertexIndex >= 4)
            heightRatio = 1.0f;
        else if (vertexIndex >= 2)
            heightRatio = 0.5f;
    }

    float sideOffset = (vertexIndex % 2 == 0) ? -0.5f : 0.5f;

    // 風
    float2 windDirection = normalize(gEnvironmentData.windDirection);
    float2 windScroll = gEnvironmentData.windOffset * gMaterial.windSpeedMultiplier;
    float2 windUV = rootPosition.xz * gMaterial.gustScale + windScroll * 0.05f;
    float gustMask = smoothstep(0.2f, 0.8f, gWindMap.SampleLevel(gLinearWrapSampler, windUV, 0).r);

    float flutter = sin(gFrameData.time * 10.0f + rootPosition.x * 1.7f + rootPosition.z * 2.3f)
                    * gMaterial.flutterAmount * gEnvironmentData.windSpeed;
    float windStrength = gustMask * gMaterial.gustStrength * gEnvironmentData.windSpeed + flutter;

    float3 windForce = float3(windDirection.x, -gMaterial.windFlattenStrength, windDirection.y) * windStrength;

    // プレイヤーなどに押し倒される
    float3 pushForce = float3(0.0f, 0.0f, 0.0f);
    float2 interactionUV = (rootPosition.xz - gInteractionData.centerWorldPos) / gInteractionData.worldSize + 0.5f;

    if (all(interactionUV >= 0.0f) && all(interactionUV <= 1.0f))
    {
        float4 interaction = gInteractionMap.SampleLevel(gLinearClampSampler, interactionUV, 0);

        float2 pushDirectionXZ = interaction.rg * 2.0f - 1.0f;
        float pushLength = length(pushDirectionXZ);
        pushDirectionXZ = (pushLength > 0.001f) ? pushDirectionXZ / pushLength : float2(0.0f, 0.0f);

        // b: 今押されている強さ, a: 通ったあとに残る跡
        float interactionWeight = max(interaction.b, interaction.a * gMaterial.trailFlattenWeight);

        float3 pushDirection = normalize(float3(pushDirectionXZ.x, -gMaterial.flattenFactor, pushDirectionXZ.y));
        pushForce = pushDirection * interactionWeight * gMaterial.interactStrength;

        // 押し倒されている間は風で揺れない
        windForce *= 1.0f - saturate(interactionWeight * 1.5f);
    }

    // 1本ごとに曲がり方を変える
    float shapeRandom1 = Hash12(rootPosition.xz * 1.13f);
    float shapeRandom2 = Hash12(rootPosition.zx * 2.71f);

    float3 tiltForce = randomForward * lerp(0.1f, 0.4f, shapeRandom1);
    float controlHeight1 = lerp(0.2f, 0.5f, shapeRandom2);
    float controlHeight2 = lerp(0.5f, 0.8f, shapeRandom1);

    float3 bendForce = windForce + pushForce + tiltForce;
    bendForce.y -= lerp(0.1f, 0.3f, shapeRandom2);

    float3 p0 = rootPosition;
    float3 p1 = rootPosition + float3(0.0f, grassHeight * controlHeight1, 0.0f) + tiltForce * (grassHeight * 0.2f);
    float3 p2 = rootPosition + float3(0.0f, grassHeight * controlHeight2, 0.0f) + bendForce * (grassHeight * 0.5f);
    float3 p3 = rootPosition + float3(0.0f, grassHeight, 0.0f) + bendForce * grassHeight;

    // 曲げても草の長さが変わらないようにする
    float3 segment1 = p1 - p0;
    float3 segment2 = p2 - p1;
    float3 segment3 = p3 - p2;
    float lengthScale = grassHeight / max(length(segment1) + length(segment2) + length(segment3), kEpsilon);
    p1 = p0 + segment1 * lengthScale;
    p2 = p1 + segment2 * lengthScale;
    p3 = p2 + segment3 * lengthScale;

    float3 centerPosition = EvaluateCubicBezier(p0, p1, p2, p3, heightRatio);
    float3 tangent = EvaluateCubicBezierTangent(p0, p1, p2, p3, heightRatio);

    float3 normal = normalize(cross(bladeRight, tangent));
    float3 right = normalize(cross(tangent, normal));

    float width = grassWidth * (1.0f - pow(heightRatio, 1.8f));
    float3 worldPosition = centerPosition + right * (sideOffset * width);

    normal = normalize(normal + right * (sideOffset * kNormalRounding));

    output.position = mul(float4(worldPosition, 1.0f), gFrameData.viewProjectionMatrix);
    output.worldPosition = worldPosition;
    output.heightRatio = heightRatio;
    output.normal = normal;
    return output;
}
