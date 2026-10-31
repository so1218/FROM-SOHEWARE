#include "Common/ShaderConstants.hlsli" 

struct SpriteVSOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    nointerpolation uint instanceId : INSTANCEID;
};

// SpriteInstance を変えたらこのファイルも再コンパイルする必要がある
ConstantBuffer<SpriteFrameConstants> gSpriteFrame : register(b0);
StructuredBuffer<SpriteInstance> gInstances : register(t0);

static const float2 kQuadCorners[6] =
{
    float2(0, 0), float2(1, 0), float2(0, 1),
    float2(1, 0), float2(1, 1), float2(0, 1)
};

SpriteVSOutput main(uint vertexId : SV_VertexID, uint instanceId : SV_InstanceID)
{
    SpriteInstance instance = gInstances[instanceId];
    float2 cornerPosition = kQuadCorners[vertexId];

    // 基準点ずらし → 拡縮
    float2 localPosition = (cornerPosition - instance.pivot) * instance.size;

    // 回転
    float cosRotation = cos(instance.rotation);
    float sinRotation = sin(instance.rotation);
    float2 rotatedPosition = float2(
        localPosition.x * cosRotation - localPosition.y * sinRotation,
        localPosition.x * sinRotation + localPosition.y * cosRotation);

    // 移動
    float2 screenPosition = rotatedPosition + instance.position;

    SpriteVSOutput output;
    output.position = mul(float4(screenPosition, 0.0f, 1.0f), gSpriteFrame.projection);
    output.texcoord = cornerPosition;
    output.instanceId = instanceId;
    return output;
}