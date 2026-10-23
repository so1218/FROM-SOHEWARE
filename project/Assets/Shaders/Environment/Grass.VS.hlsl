#include "Common/Object3D.hlsli"
#include "Common/ShaderConstants.hlsli"
#include "Common/MathUtils.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<GrassMaterialData> gMaterial : register(b5);
ConstantBuffer<GrassCullingData> gGrassCullingData : register(b6);
ConstantBuffer<GlobalEnvironmentData> gEnvironmentData : register(b7);
ConstantBuffer<InteractionConstants> gInteractionData : register(b8);
StructuredBuffer<GrassInstanceData> gInstanceData : register(t10);

// 風の強度マップ
Texture2D<float> gWindMap : register(t11);
// インタラクションマップ
Texture2D<float4> gInteractionMap : register(t12);

SamplerState gLinearWrapSampler : register(s2);
SamplerState gLinearClampSampler : register(s3);

// ブレード1本あたりの頂点数 (TriangleStrip描画: 8頂点 = 3セグメント)
#define NUM_VERTICES_PER_BLADE 8

// 3次ベジェ曲線の座標評価
float3 EvaluateCubicBezier(float3 p0, float3 p1, float3 p2, float3 p3, float t)
{
    float u = 1.0f - t;
    float u2 = u * u;
    float u3 = u2 * u;
    float t2 = t * t;
    float t3 = t2 * t;
    
    return u3 * p0 + (3.0f * u2 * t) * p1 + (3.0f * u * t2) * p2 + t3 * p3;
}

// 3次ベジェ曲線の接線評価
float3 EvaluateCubicBezierTangent(float3 p0, float3 p1, float3 p2, float3 p3, float t)
{
    float u = 1.0f - t;
    float u2 = u * u;
    float t2 = t * t;
    
    float3 tangent = -3.0f * u2 * p0 +
                     (3.0f * u2 - 6.0f * u * t) * p1 +
                     (6.0f * u * t - 3.0f * t2) * p2 +
                     3.0f * t2 * p3;
    return normalize(tangent);
}

// Packed Color (RGBA8) -> float4 (0.0~1.0) 展開
float4 UnpackColor(uint packedColor)
{
    return float4(
        (packedColor & 0xFF) / 255.0f,
        ((packedColor >> 8) & 0xFF) / 255.0f,
        ((packedColor >> 16) & 0xFF) / 255.0f,
        ((packedColor >> 24) & 0xFF) / 255.0f
    );
}

struct GrassPSInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 worldPosition : WORLD_POSITION;
    float4 color : COLOR;
};

GrassPSInput main(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
    GrassPSInput output;
    
    // CSのカリングパスを通過した有効なインスタンスのみ
    GrassInstanceData instance = gInstanceData[instanceID];
    
    float3 rootPos = instance.posAndHeight.xyz;
    float grassHeight = instance.posAndHeight.w;
    float rotationY = instance.rotWidthColor.x;
    float grassWidth = instance.rotWidthColor.y;
    float4 instanceColor = UnpackColor(asuint(instance.rotWidthColor.z));

    float distToCam = distance(rootPos, gFrameData.cameraWorldPosition);

    // 遠景のシルエット維持 (ハゲ防止)
    float thinFactor = saturate((distToCam - gGrassCullingData.thinStartDistance) /
                                max(gGrassCullingData.maxDrawDistance - gGrassCullingData.thinStartDistance, kEpsilon));
    grassWidth *= lerp(1.0f, gGrassCullingData.maxWidthMultiplier, thinFactor);

    // 基底ベクトルの算出
    float s, c;
    sincos(rotationY, s, c);
    float3 randomRight = float3(c, 0.0f, -s);

    // Y軸ビルボードブレンド
    float3 toCamera = normalize(float3(gFrameData.cameraWorldPosition.x - rootPos.x, 0.0f, gFrameData.cameraWorldPosition.z - rootPos.z));
    float3 faceCameraRight = normalize(cross(toCamera, float3(0.0f, 1.0f, 0.0f)));
    float3 baseRight = normalize(lerp(randomRight, faceCameraRight, 0.6f));

    uint vertexIdx = vertexID % NUM_VERTICES_PER_BLADE;
    
    // 現在の頂点がブレードのどの高さに位置するか (0.0:根元 ~ 1.0:先端)
    float t = (vertexIdx / 2) / (float) ((NUM_VERTICES_PER_BLADE / 2) - 1);

    // 距離ベースLOD
    if (distToCam > gGrassCullingData.lodDistance2)
    {
        if (vertexIdx >= 2)
            t = 1.0f; // LOD2: 1セグメント化
    }
    else if (distToCam > gGrassCullingData.lodDistance1)
    {
        if (vertexIdx >= 4)
            t = 1.0f; // LOD1: 2セグメント化
        else if (vertexIdx >= 2)
            t = 0.5f;
    }

    // 左右のオフセット
    float sideOffset = (vertexIdx % 2 == 0) ? -0.5f : 0.5f;

    // -------------------------------------------------------------------------
    // 風・インタラクション
    // -------------------------------------------------------------------------
    float2 windDir = normalize(gEnvironmentData.windDirection);
    float2 windOffset = gEnvironmentData.windOffset * gMaterial.windSpeedMultiplier;
    float currentWindMag = gEnvironmentData.windSpeed * gMaterial.windStrengthMultiplier;
    
    float2 windUV = (rootPos.xz * gMaterial.gustScale) + windOffset * 0.05f;
    float gustNoise = gWindMap.SampleLevel(gLinearWrapSampler, windUV, 0).r;
    float gustMask = smoothstep(0.2f, 0.8f, gustNoise);
    
    float flutter = sin(gFrameData.gTime * 10.0f + (rootPos.x * 1.7f + rootPos.z * 2.3f))
                    * gMaterial.flutterAmount * gEnvironmentData.windSpeed;
    float totalWindMag = currentWindMag + (gustMask * gMaterial.gustStrength * gEnvironmentData.windSpeed) + flutter;
    
    float3 windForce = float3(windDir.x * totalWindMag, -totalWindMag * gMaterial.windFlattenStrength, windDir.y * totalWindMag);

    // -------------------------------------------------------------------------
    // エンティティ干渉
    // -------------------------------------------------------------------------
    float3 pushForce = float3(0.0f, 0.0f, 0.0f);
    float2 interactUV = (rootPos.xz - gInteractionData.centerWorldPos) / gInteractionData.worldSize + 0.5f;

    if (all(interactUV >= 0.0f) && all(interactUV <= 1.0f))
    {
        float4 interactData = gInteractionMap.SampleLevel(gLinearClampSampler, interactUV, 0);

        float2 pushDirXZ = (interactData.rg * 2.0f) - 1.0f;
        float dirLen = length(pushDirXZ);
        pushDirXZ = (dirLen > 0.001f) ? (pushDirXZ / dirLen) : float2(0.0f, 0.0f);

        float instantPower = interactData.b;
        float trailPower = interactData.a * gMaterial.trailFlattenWeight;
        float totalInteractWeight = max(instantPower, trailPower);

        float3 pushDir = float3(pushDirXZ.x, -gMaterial.flattenFactor, pushDirXZ.y);
        pushForce = normalize(pushDir) * totalInteractWeight * gMaterial.interactStrength;

        windForce *= (1.0f - saturate(totalInteractWeight * 1.5f));
    }

    // -------------------------------------------------------------------------
    // 形状構築 (ベジェ曲線)
    // -------------------------------------------------------------------------
    float3 randomForward = float3(s, 0.0f, c);
    float randShape1 = Hash12(rootPos.xz * 1.13f);
    float randShape2 = Hash12(rootPos.zx * 2.71f);

    float3 tiltForce = randomForward * lerp(0.1f, 0.4f, randShape1);
    float cp1Height = lerp(0.2f, 0.5f, randShape2);
    float cp2Height = lerp(0.5f, 0.8f, randShape1);

    float3 totalForce = windForce + pushForce;
    totalForce.y -= lerp(0.1f, 0.3f, randShape2);

    float3 p0 = rootPos;
    float3 p1 = rootPos + float3(0.0f, grassHeight * cp1Height, 0.0f) + tiltForce * (grassHeight * 0.2f);
    float3 p2 = rootPos + float3(0.0f, grassHeight * cp2Height, 0.0f) + (totalForce + tiltForce) * (grassHeight * 0.5f);
    float3 p3 = rootPos + float3(0.0f, grassHeight, 0.0f) + (totalForce + tiltForce) * grassHeight;

    // 長さの維持
    float3 v1 = p1 - p0;
    float3 v2 = p2 - p1;
    float3 v3 = p3 - p2;
    float preserveScale = grassHeight / max(length(v1) + length(v2) + length(v3), kEpsilon);
    
    p1 = p0 + v1 * preserveScale;
    p2 = p1 + v2 * preserveScale;
    p3 = p2 + v3 * preserveScale;

    // -------------------------------------------------------------------------
    // ジオメトリ＆カラー評価
    // -------------------------------------------------------------------------
    float3 centerPos = EvaluateCubicBezier(p0, p1, p2, p3, t);
    float3 tangent = EvaluateCubicBezierTangent(p0, p1, p2, p3, t);
    
    float3 proceduralNormal = normalize(cross(baseRight, tangent));
    float3 trueRight = normalize(cross(tangent, proceduralNormal));
    
    float currentWidth = grassWidth * (1.0f - pow(t, 1.8f));
    float3 worldPos = centerPos + trueRight * (sideOffset * currentWidth);

    proceduralNormal = normalize(proceduralNormal + trueRight * (sideOffset * 2.0f * 0.25f));

    // カラー計算 
    float3 finalColor = lerp(gMaterial.rootColor, gMaterial.tipColor, t);

    // 出力
    output.position = mul(float4(worldPos, 1.0f), gFrameData.viewProjectionMatrix);
    output.worldPosition = worldPos;
    output.texcoord = float2(sideOffset + 0.5f, t);
    output.normal = proceduralNormal;
    output.tangent = tangent;
    output.color = float4(finalColor, gustMask); // W要素に突風マスクを格納
  
    return output;
}