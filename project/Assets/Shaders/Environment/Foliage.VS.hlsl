#include "Common/ShaderConstants.hlsli"

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

// テクスチャフェッチ(WindMap)を用いず、ワールド座標と時間をベースにした合成サイン波で風を近似する。
// メモリ帯域を節約しつつ、Sway(全体的な揺れ)、Flutter(葉の微振動)、Gust(突風)のレイヤーを重ねて自然な動きを表現。
float3 CalculateWindDisplacement(float3 worldPos, float windWeight, float3 basePos)
{
    float3 windDir = normalize(float3(gEnvironmentData.windDirection.x, 0.0f, gEnvironmentData.windDirection.y));
    float windSpeed = gEnvironmentData.windSpeed;

    // インスタンスごとに位相をずらすため、基準座標(basePos)を用いて位相オフセットを計算
    float phase = dot(basePos.xz, float2(0.1f, 0.1f)) + (gEnvironmentData.windTime * windSpeed);
    
    // 根元は硬く、先端にいくほど柔らかく曲がるように累乗でカーブを付ける
    float bentWeight = pow(windWeight, gMaterial.stiffness);
    
    float sway = sin(phase) * 0.5f + 0.5f;
    float flutter = sin(phase * gMaterial.flutterSpeed * 3.1415f) * gMaterial.flutterScale;
    
    float gustPhase = dot(basePos.xz, float2(0.05f, 0.05f)) + (gEnvironmentData.windTime * windSpeed * gEnvironmentData.windTurbulence);
    float gust = saturate(sin(gustPhase) * 0.5f + 0.5f);

    float totalDisplacement = (sway + flutter * gust) * bentWeight * gMaterial.windResponse;
    
    return windDir * totalDisplacement;
}

FoliageVSOutput main(FoliageVSInput input)
{
    FoliageVSOutput output;
    
    FoliageInstanceData instance = gInstanceData[input.instanceID];
    
    float3 basePos = instance.posAndScale.xyz;
    float scale = instance.posAndScale.w;
    float4 quat = instance.rotationQuat;
    float3 localPos = input.position.xyz;
    
    // DCCツール側で「揺れやすさ」を頂点カラーとしてペイントするアーティストの工数を削減するため、
    // ローカルのY座標からプロシージャルに揺れウェイトを算出。
    // plantHeightを基準にすることで、草のスケールに関わらず0.0(根元)～1.0(先端)のグラデーションを得る。
    float windWeight = saturate(localPos.y / max(gMaterial.plantHeight, 0.001f));
    localPos *= scale;
    float3 rotatedPos = RotateVectorByQuat(localPos, quat);
    float3 worldPos = basePos + rotatedPos;

    // 風による変位を計算
    float3 windDisp = CalculateWindDisplacement(worldPos, windWeight, basePos);
    
    // インタラクションによる変位を計算
    float3 pushDisp = float3(0, 0, 0);
    float2 interactUV = (basePos.xz - gInteractionData.centerWorldPos) / gInteractionData.worldSize + 0.5f;
    
    if (all(interactUV >= 0.0f) && all(interactUV <= 1.0f))
    {
        float4 interactData = gInteractionMap.SampleLevel(gLinearClampSampler, interactUV, 0);
        
        float2 pushDirXZ = (interactData.rg * 2.0f) - 1.0f;
        float dirLen = length(pushDirXZ);
        pushDirXZ = (dirLen > 0.001f) ? (pushDirXZ / dirLen) : float2(0, 0);

        // 瞬間的な力と、痕跡(Trail)の合成
        float instantPower = interactData.b;
        float trailPower = interactData.a * gMaterial.trailFlattenWeight; // 植物ごとのウェイトを適用
        float totalInteractWeight = max(instantPower, trailPower);

        // 押し出し方向 (flattenFactor が低いほど横に逃げるだけになる)
        float3 pushDir = float3(pushDirXZ.x, -gMaterial.flattenFactor, pushDirXZ.y);
        
        // 根元は動かさず、先端ほど強く押し出されるように windWeight を掛ける
        pushDisp = normalize(pushDir) * totalInteractWeight * gMaterial.interactStrength * windWeight;
        
        // 干渉されている間は風の揺れを抑える
        windDisp *= saturate(1.0f - (totalInteractWeight * 1.5f));
    }

    // 風とインタラクションの変位を合成
    worldPos += (windDisp + pushDisp);

    // 長さの維持
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
    
    // TAA (Temporal Anti-Aliasing) やモーションブラー向けに Velocity(Motion Vector) を計算。
    // 負荷軽減のため、風による微細なフレーム間頂点移動は無視し、カメラインプットの差分のみで近似している。
    float4 prevClipPos = mul(float4(basePos + rotatedPos, 1.0f), gFrameData.prevViewProj);
    
    output.position = clipPos;
    output.velocity = (clipPos.xy / clipPos.w - prevClipPos.xy / prevClipPos.w) * float2(0.5f, -0.5f);

    output.texcoord = input.texcoord;
    output.color = float4(windWeight, 0.0f, 0.0f, 0.0f);
    output.instanceTint = instance.colorVariation;

    return output;
}