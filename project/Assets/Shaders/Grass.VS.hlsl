#include "Object3D.hlsli"
#include "ShaderConstants.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);

cbuffer DirectionalLights : register(b1)
{
    DirectionalLight gDirectionalLights[MAX_DIRECTIONAL_LIGHTS];
};

ConstantBuffer<MaterialData> gMaterial : register(b5);

// インスタンスデータ
StructuredBuffer<GrassInstanceData> gInstanceData : register(t10);

struct VertexInput
{
    float4 position : POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL;
    uint instanceID : SV_InstanceID;
};

struct PixelInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
    float3 normal : NORMAL;
    float3 worldPosition : WORLD_POSITION;
    float4 color : COLOR;
    float4 shadowCoord : SHADOW_COORD;
};
PixelInput main(VertexInput input)
{
    PixelInput output;

    GrassInstanceData instance = gInstanceData[input.instanceID];
    float4 worldPos = mul(input.position, instance.world);

  // =================================================
    // 【追加①】元の長さを記録しておく
    // =================================================
    // この頂点の根本の座標（ローカルのY=0地点）をワールド座標に変換
    float3 localRoot = float3(input.position.x, 0.0f, input.position.z);
    float3 rootWorldPos = mul(float4(localRoot, 1.0f), instance.world).xyz;
    
    // 根本から現在の頂点までの「本来の長さ」を計算して保存
    float originalLength = distance(worldPos.xyz, rootWorldPos);
    // =================================================

    // 根本は動かさず、先端ほど大きく動くウェイト
    float windWeight = 1.0f - input.texcoord.y;
    
    // -------------------------------------------------
    // ① 風の計算 (既存)
    // -------------------------------------------------
    float windSpeed = gFrameData.gTime * gMaterial.grassWindSpeed;
    float windPhase = worldPos.x * 0.5f + worldPos.z * 0.5f;
   
    float windSway = sin(windSpeed + windPhase) * gMaterial.grassWindAmplitude;
    worldPos.x += windSway * windWeight;
    worldPos.z += cos(windSpeed + windPhase) * (gMaterial.grassWindAmplitude * 0.4f) * windWeight;

    // -------------------------------------------------
    // ② プレイヤーによる踏み込み計算 (既存)
    // -------------------------------------------------
    float3 diff = worldPos.xyz - gMaterial.playerPos;
    float distXZ = length(diff.xz);

    if (distXZ < gMaterial.interactRadius)
    {
        float interactWeight = 1.0f - saturate(distXZ / gMaterial.interactRadius);
        interactWeight = smoothstep(0.0f, 1.0f, interactWeight);

        float3 pushDir = normalize(float3(diff.x, 0.001f, diff.z));

        // 外側に押し倒す (XZ)
        worldPos.xyz += pushDir * interactWeight * gMaterial.interactStrength * windWeight;
        
        // 下に押し潰す (Y軸)
        worldPos.y -= interactWeight * (gMaterial.interactStrength * 0.5f) * windWeight;
    }

    // =================================================
    // 【追加②】長さの補正（ストレッチ防止！）
    // =================================================
    // 風や踏み込みで動かした後の座標から、根本への方向ベクトルを作る
    if (originalLength > 0.001f) // 根本の頂点でのゼロ除算(NaN)を防ぐための安全策
    {
        float3 dirFromRoot = normalize(worldPos.xyz - rootWorldPos);
        
        // 根本の座標から、計算した方向に向かって「本来の長さ」分だけ伸ばした位置を最終座標にする
        worldPos.xyz = rootWorldPos + dirFromRoot * originalLength;
    }
    // =================================================

    output.position = mul(worldPos, gFrameData.viewProjectionMatrix);
    output.shadowCoord = mul(worldPos, gDirectionalLights[0].viewProj);
    output.worldPosition = worldPos.xyz;
    output.texcoord = input.texcoord;
    
    // 法線のフェイク (既存)
    float3 worldNormal = mul(input.normal, (float3x3) instance.world);
    worldNormal = normalize(lerp(worldNormal, float3(0.0f, 1.0f, 0.0f), gMaterial.grassNormalBlend));
    output.normal = worldNormal;

    output.color = instance.color;

    return output;
}