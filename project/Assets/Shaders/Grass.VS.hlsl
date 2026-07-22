#include "Object3D.hlsli"
#include "ShaderConstants.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<GrassMaterialData> gMaterial : register(b5);
StructuredBuffer<GrassInstanceData> gInstanceData : register(t10);

Texture2D<float> gWindMap : register(t11); // 風の強さを表すグレースケールノイズ画像
SamplerState gLinearWrapSampler : register(s2);

// 1枚の草（ブレード）を構成する頂点数。TriangleStripで描画 (8頂点 = 3セグメント)
#define NUM_VERTICES_PER_BLADE 8

// ---------------------------------------------------------
// ヘルパー関数
// ---------------------------------------------------------
float3 EvaluateCubicBezier(float3 p0, float3 p1, float3 p2, float3 p3, float t)
{
    float u = 1.0f - t;
    float u2 = u * u;
    float u3 = u2 * u;
    float t2 = t * t;
    float t3 = t2 * t;
    
    return u3 * p0 + (3.0f * u2 * t) * p1 + (3.0f * u * t2) * p2 + t3 * p3;
}

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

// 32bit UInt を float4 に解凍する関数
float4 UnpackColor(uint packedColor)
{
    return float4(
        (packedColor & 0xFF) / 255.0f,
        ((packedColor >> 8) & 0xFF) / 255.0f,
        ((packedColor >> 16) & 0xFF) / 255.0f,
        ((packedColor >> 24) & 0xFF) / 255.0f
    );
}

float Hash12(float2 p)
{
    float3 p3 = frac(float3(p.xyx) * 0.1031f);
    p3 += dot(p3, p3.yzx + 33.33f);
    return frac((p3.x + p3.y) * p3.z);
}

struct PixelInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 worldPosition : WORLD_POSITION;
    float4 color : COLOR;
    float4 currentClipPos : POSITION1;
    float4 prevClipPos : POSITION2;
};

PixelInput main(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
    PixelInput output;
    GrassInstanceData instance = gInstanceData[instanceID];
    
    float3 rootPos = instance.posAndHeight.xyz;
    float grassHeight = instance.posAndHeight.w;
    float rotationY = instance.rotWidthColor.x;
    float grassWidth = instance.rotWidthColor.y;
    float4 instanceColor = UnpackColor(asuint(instance.rotWidthColor.z));

    // 草の根本の基底ベクトル
    float s, c;
    sincos(rotationY, s, c);
    float3 baseRight = float3(c, 0.0f, -s);

    uint vertexIdx = vertexID % NUM_VERTICES_PER_BLADE;
    float t = (vertexIdx / 2) / (float) ((NUM_VERTICES_PER_BLADE / 2) - 1);
    float sideOffset = (vertexIdx % 2 == 0) ? -0.5f : 0.5f;

    // ==========================================
    // 1. 風の計算 (Fluid Wind Field)
    // ==========================================
    float2 windDir = normalize(gMaterial.windDir);
    float windTime = gFrameData.gTime * gMaterial.windSpeed;
    
    // Gust (突風の滑らかなうねり)
    float2 windUV = (rootPos.xz * gMaterial.gustScale) - windDir * windTime * 0.05f;
    float gustNoise = gWindMap.SampleLevel(gLinearWrapSampler, windUV, 0).r;
    float gustMask = smoothstep(0.2f, 0.8f, gustNoise);
    
    // Flutter (高周波な細かな揺れ)
    float flutterPhase = rootPos.x * 1.7f + rootPos.z * 2.3f;
    float flutter = sin(gFrameData.gTime * 10.0f + flutterPhase) * gMaterial.flutterAmount;
    
    // 合計の風力ベクトル
    float totalWindMag = gMaterial.baseWindStrength + (gustMask * gMaterial.gustStrength) + flutter;
    float flattenForce = totalWindMag * gMaterial.windFlattenStrength;
    float3 windForce = float3(windDir.x * totalWindMag, -flattenForce, windDir.y * totalWindMag);

    // ==========================================
    // 2. プレイヤーの押し倒し (Interaction)
    // ==========================================
    float3 diff = rootPos - gMaterial.playerPos;
    float distXZ = length(diff.xz);
    float3 pushForce = float3(0.0f, 0.0f, 0.0f);
    
    if (distXZ < gMaterial.interactRadius)
    {
        float weight = 1.0f - saturate(distXZ / gMaterial.interactRadius);
        weight = smoothstep(0.0f, 1.0f, weight);
        
        float3 pushDir = normalize(float3(diff.x, -0.6f, diff.z));
        pushForce = pushDir * weight * gMaterial.interactStrength;
        
        // 押し倒された部分は風の力を減衰
        windForce *= (1.0f - weight);
    }

    // ==========================================
    // 3. ベジェ曲線＆長さの保持 (Length Preservation)
    // ==========================================
    float3 p0 = rootPos;
    float3 p1 = rootPos + float3(0.0f, grassHeight * 0.35f, 0.0f);
    
    float3 totalForce = windForce + pushForce;
    totalForce.y -= 0.15f; // 自重による下方向の垂れ下がり
    
    float3 p2 = rootPos + float3(0.0f, grassHeight * 0.7f, 0.0f) + totalForce * (grassHeight * 0.5f);
    float3 p3 = rootPos + float3(0.0f, grassHeight, 0.0f) + totalForce * grassHeight;

    // 伸びすぎ防止のスケール補正
    float curveLength = distance(p0, p1) + distance(p1, p2) + distance(p2, p3);
    float preserveScale = grassHeight / max(curveLength, 0.001f);
    
    p1 = p0 + (p1 - p0) * preserveScale;
    p2 = p1 + (p2 - p1) * preserveScale;
    p3 = p2 + (p3 - p2) * preserveScale;

    // ==========================================
    // 4. メッシュ・法線の構築
    // ==========================================
    float3 centerPos = EvaluateCubicBezier(p0, p1, p2, p3, t);
    float3 tangent = EvaluateCubicBezierTangent(p0, p1, p2, p3, t);
    
    float3 proceduralNormal = normalize(cross(baseRight, tangent));
    float3 trueRight = normalize(cross(tangent, proceduralNormal));
    
    float widthFactor = 1.0f - pow(t, 1.8f);
    float currentWidth = grassWidth * widthFactor;
    
    float3 worldPos = centerPos + trueRight * (sideOffset * currentWidth);

    // 円柱フェイク法線 (極端になり過ぎないよう 0.25f に抑制)
    float normalBend = sideOffset * 2.0f;
    proceduralNormal = normalize(proceduralNormal + trueRight * normalBend * 0.25f);

    // 草原全体の色ムラ (World-space Color Variation)
    float randVal = Hash12(rootPos.xz * 0.1f);
    float3 baseColor = instanceColor.rgb * lerp(1.0f, 0.85f + randVal * 0.3f, gMaterial.colorVariation);

    // 出力設定
    output.position = mul(float4(worldPos, 1.0f), gFrameData.viewProjectionMatrix);
    output.worldPosition = worldPos;
    output.texcoord = float2(sideOffset + 0.5f, t);
    output.normal = proceduralNormal;
    output.tangent = tangent;
    
    // Alpha に「突風の強度 (gustMask)」を乗せて PS に渡す
    output.color = float4(baseColor, gustMask);
    
    output.currentClipPos = output.position;
    output.prevClipPos = mul(float4(worldPos, 1.0f), gFrameData.prevViewProj);

    return output;
}