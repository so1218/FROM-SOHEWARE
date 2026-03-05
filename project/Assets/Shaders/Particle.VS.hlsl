#include "ParticleCommon.hlsli" 
#include "ShaderConstants.hlsli" 

struct VertexIn
{
    float4 position : POSITION; 
    float2 uv : TEXCOORD; 
};

// インスタンシング用のデータを格納するためのバッファ
StructuredBuffer<ParticleInstanceData> instanceBuffer : register(t0);
ConstantBuffer<FrameData> gFrameData : register(b0);

struct VertexOut
{
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD;
    float4 color : COLOR;
    float textureIndex : TEXCOORD1;
};

VertexOut main(VertexIn vin, uint instanceId : SV_InstanceID)
{
    VertexOut vout;
    ParticleInstanceData inst = instanceBuffer[instanceId];

    float3 worldPos;

    // Z軸回転の計算
    float cosR = cos(inst.rotationZ);
    float sinR = sin(inst.rotationZ);
    float rotatedX = vin.position.x * cosR - vin.position.y * sinR;
    float rotatedY = vin.position.x * sinR + vin.position.y * cosR;

    if (inst.isBillboard == 1)
    {
        // ビルボード処理
        float3 right = gFrameData.cameraRight;
        float3 up = gFrameData.cameraUp;

        worldPos = inst.position + (rotatedX * right * inst.scale.x) + (rotatedY * up * inst.scale.y);
    }
    else
    {
        // ビルボードではない場合
        worldPos = inst.position + float3(rotatedX * inst.scale.x, rotatedY * inst.scale.y, 0.0f);
    }

    vout.svpos = mul(float4(worldPos, 1.0f), gFrameData.viewProjectionMatrix);
    vout.uv = vin.uv;
    vout.color = inst.color;
    vout.color.rgb *= inst.intensity;
    vout.textureIndex = inst.textureIndex;

    return vout;
}