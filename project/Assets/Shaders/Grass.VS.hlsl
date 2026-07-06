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
    float4 currentClipPos : POSITION1;
    float4 prevClipPos : POSITION2;
};

PixelInput main(VertexInput input)
{
    PixelInput output;

    GrassInstanceData instance = gInstanceData[input.instanceID];
    float4 worldPos = mul(input.position, instance.world);
    
    // 元の長さを記録
    float3 localRoot = float3(input.position.x, 0.0f, input.position.z);
    float3 rootWorldPos = mul(float4(localRoot, 1.0f), instance.world).xyz;
    float originalLength = distance(worldPos.xyz, rootWorldPos);

    // 先端ほど大きく動くウェイト
    float windWeight = 1.0f - input.texcoord.y;
    
    // プレイヤーへの距離を風の計算より先に
    float3 diff = worldPos.xyz - gMaterial.playerPos;
    float distXZ = length(diff.xz);
    float interactWeight = 0.0f; // 踏まれている度合い

    if (distXZ < gMaterial.interactRadius)
    {
        interactWeight = 1.0f - saturate(distXZ / gMaterial.interactRadius);
        interactWeight = smoothstep(0.0f, 1.0f, interactWeight);
    }

    // 踏まれている度合いの逆
    float windDampening = 1.0f - interactWeight;
    
    // 風の計算
    float windSpeed = gFrameData.gTime * gMaterial.grassWindSpeed;
    float windPhase = worldPos.x * 0.5f + worldPos.z * 0.5f;
   
    float windSway = sin(windSpeed + windPhase) * gMaterial.grassWindAmplitude;
    
    // 踏まれている草は風の影響を消す
    worldPos.x += windSway * windWeight * windDampening;
    worldPos.z += cos(windSpeed + windPhase) * (gMaterial.grassWindAmplitude * 0.4f) * windWeight * windDampening;

    // プレイヤーによる踏み込み計算
    if (interactWeight > 0.0f)
    {
        float3 pushDir = normalize(float3(diff.x, 0.001f, diff.z));

        // 外側に押し倒す
        worldPos.xyz += pushDir * interactWeight * gMaterial.interactStrength * windWeight;
        
        // 下に押し潰す
        worldPos.y -= interactWeight * (gMaterial.interactStrength * 0.5f) * windWeight;
    }
    
    // 長さの補正
    if (originalLength > 0.001f)
    {
        float3 dirFromRoot = normalize(worldPos.xyz - rootWorldPos);
        worldPos.xyz = rootWorldPos + dirFromRoot * originalLength;
    }

    output.position = mul(float4(worldPos.xyz, 1.0f), gFrameData.viewProjectionMatrix);
    output.currentClipPos = output.position;
    
    float3 prevWorldPos = worldPos.xyz;

    output.prevClipPos = mul(float4(prevWorldPos.xyz, 1.0f), gFrameData.prevViewProj);
    
    output.worldPosition = worldPos.xyz;
    output.texcoord = input.texcoord;
    
    // 法線のフェイク
    float3 worldNormal = mul(input.normal, (float3x3) instance.world);
    worldNormal = normalize(lerp(worldNormal, float3(0.0f, 1.0f, 0.0f), gMaterial.grassNormalBlend));
    output.normal = worldNormal;

    output.color = instance.color;

    return output;
}
