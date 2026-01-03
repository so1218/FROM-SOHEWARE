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

    // インスタンスデータを取得
    ParticleInstanceData inst = instanceBuffer[instanceId];

    float4x4 world = inst.worldMatrix;
    float3 worldPos;

    if (inst.isBillboard == 1)
    {
        // ビルボード中心位置
        float3 center = world[3].xyz;

        // ワールド行列からスケールを取得
        float scaleX = length(world[0].xyz);
        float scaleY = length(world[1].xyz);

        // カメラ基準の右方向と上方向
        float3 right = gFrameData.cameraRight;
        float3 up = gFrameData.cameraUp;

        // Z軸回転を適用
        float cosR = cos(inst.rotationZ);
        float sinR = sin(inst.rotationZ);

        float rotatedX = vin.position.x * cosR - vin.position.y * sinR;
        float rotatedY = vin.position.x * sinR + vin.position.y * cosR;

        // カメラ向きに板ポリゴンを配置
        worldPos = center + rotatedX * right * scaleX + rotatedY * up * scaleY;
    }
    else
    {
        // 通常のワールド変換
        worldPos = mul(vin.position, world).xyz;
    }

    // ワールド座標からクリップ空間へ変換
    vout.svpos = mul(float4(worldPos, 1.0f), gFrameData.viewProjectionMatrix);

    // 頂点属性を設定
    vout.uv = vin.uv;
    vout.color = inst.color;
    vout.color.rgb *= inst.intensity;
    vout.textureIndex = inst.textureIndex;

    return vout;
}