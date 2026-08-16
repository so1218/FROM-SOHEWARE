#include "ShaderConstants.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<GlobalEnvironmentData> gEnvironmentData : register(b4);

ConstantBuffer<FoliageMaterialData> gMaterial : register(b5);

// ★ インスタンスデータ (位置、回転、マテリアルIDなど)
StructuredBuffer<FoliageInstanceData> gInstanceData : register(t10);

struct FoliageVSInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
    float3 tangent : TANGENT0;
    uint instanceID : SV_InstanceID;
};

struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float3 worldPosition : WORLD_POSITION;
    float2 texcoord : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float4 color : COLOR0; // PSでの Gust Mask (突風マスク) 等に使用
    float3 instanceTint : COLOR1; // RGB 色ムラ
    float2 velocity : TEXCOORD1;
};

// クォータニオン回転
float3 RotateVectorByQuat(float3 v, float4 q)
{
    float3 t = 2.0f * cross(q.xyz, v);
    return v + q.w * t + cross(q.xyz, t);
}

// -----------------------------------------------------------------------------
// ★ 高速なプロシージャル風計算
// -----------------------------------------------------------------------------
float3 CalculateWindDisplacement(float3 worldPos, float windWeight, float3 basePos)
{
    float3 windDir = normalize(float3(gEnvironmentData.windDirection.x, 0.0f, gEnvironmentData.windDirection.y));
    float windSpeed = gEnvironmentData.windSpeed;

    float phase = dot(basePos.xz, float2(0.1f, 0.1f)) + (gEnvironmentData.windTime * windSpeed);
    
    // 定数バッファの gMaterial を直接使用
    float bentWeight = pow(windWeight, gMaterial.stiffness);
    float sway = sin(phase) * 0.5f + 0.5f;
    float flutter = sin(phase * gMaterial.flutterSpeed * 3.1415f) * gMaterial.flutterScale;
    
    float gustPhase = dot(basePos.xz, float2(0.05f, 0.05f)) + (gEnvironmentData.windTime * windSpeed * gEnvironmentData.windTurbulence);
    float gust = saturate(sin(gustPhase) * 0.5f + 0.5f);

    float totalDisplacement = (sway + flutter * gust) * bentWeight * gMaterial.windResponse;
    
    return windDir * totalDisplacement;
}

VertexShaderOutput main(FoliageVSInput input)
{
    VertexShaderOutput output;
    
    FoliageInstanceData instance = gInstanceData[input.instanceID];
    
    float3 basePos = instance.posAndScale.xyz;
    float scale = instance.posAndScale.w;
    float4 quat = instance.rotationQuat;
    float3 localPos = input.position.xyz;
    
    // ==========================================================
    // ★ 変更部分: 頂点カラーではなく、ローカルの高さから揺れウェイトを自動生成
    // ==========================================================
    // 例: input.position.y が 0.0(根元) ～ 1.0(先端) の場合、plantHeight が 1.0 なら
    // windWeight は 0.0 ～ 1.0 のグラデーションになる。
    float windWeight = saturate(localPos.y / max(gMaterial.plantHeight, 0.001f));
    
    // (オプション) もし揺れ方をカーブさせたい場合は累乗する
    // windWeight = pow(windWeight, 1.5f); 

    localPos *= scale;
    float3 rotatedPos = RotateVectorByQuat(localPos, quat);
    float3 worldPos = basePos + rotatedPos;

    // 引数から mat を削除（関数内でグローバルCBを参照）
    float3 windDisp = CalculateWindDisplacement(worldPos, windWeight, basePos);
    worldPos += windDisp;

    // 長さ維持
    float currentLen = length(worldPos - basePos);
    float originalLen = length(rotatedPos);
    if (currentLen > 0.001f)
    {
        worldPos = basePos + (worldPos - basePos) * (originalLen / currentLen);
    }

    output.worldPosition = worldPos;

    float3 localNormal = normalize(input.normal);
    float3 localTangent = normalize(input.tangent);
    output.normal = normalize(RotateVectorByQuat(localNormal, quat));
    output.tangent = normalize(RotateVectorByQuat(localTangent, quat));

    float4 clipPos = mul(float4(worldPos, 1.0f), gFrameData.viewProjectionMatrix);
    float4 prevClipPos = mul(float4(basePos + rotatedPos, 1.0f), gFrameData.prevViewProj);
    
    output.position = clipPos;
    output.velocity = (clipPos.xy / clipPos.w - prevClipPos.xy / prevClipPos.w) * float2(0.5f, -0.5f);

    output.texcoord = input.texcoord;
    output.color = float4(windWeight, 0.0f, 0.0f, 0.0f);
    output.instanceTint = instance.colorVariation;

    return output;
}