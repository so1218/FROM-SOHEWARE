#include "ShaderConstants.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<GlobalEnvironmentData> gEnvironmentData : register(b4);
ConstantBuffer<FoliageMaterialData> gMaterial : register(b5);

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

VertexShaderOutput main(FoliageVSInput input)
{
    VertexShaderOutput output;
    
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

    float3 windDisp = CalculateWindDisplacement(worldPos, windWeight, basePos);
    worldPos += windDisp;

    // 長さの維持 (Length Preservation)
    // 単純なベクトル加算で頂点をオフセットすると、草が不自然に伸びてしまう(スケールしてしまう)。
    // これを避けるため、ピボット(basePos)からの距離を元の長さに正規化し、弧を描いて曲がるように補正する。
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