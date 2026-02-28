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

    // 風の計算
    float windWeight = 1.0f - input.texcoord.y;
    
    float windSpeed = gFrameData.gTime * gMaterial.grassWindSpeed;
    float windPhase = worldPos.x * 0.5f + worldPos.z * 0.5f;
   
    float windSway = sin(windSpeed + windPhase) * gMaterial.grassWindAmplitude;
    
    worldPos.x += windSway * windWeight;
    // Z軸の揺れ 
    worldPos.z += cos(windSpeed + windPhase) * (gMaterial.grassWindAmplitude * 0.4f) * windWeight;

    output.position = mul(worldPos, gFrameData.viewProjectionMatrix);
    output.shadowCoord = mul(worldPos, gDirectionalLights[0].viewProj);
    output.worldPosition = worldPos.xyz;
    output.texcoord = input.texcoord;
    
    // 法線のフェイク
    float3 worldNormal = mul(input.normal, (float3x3) instance.world);

    worldNormal = normalize(lerp(worldNormal, float3(0.0f, 1.0f, 0.0f), gMaterial.grassNormalBlend));
    output.normal = worldNormal;

    output.color = instance.color;

    return output;
}