#include "ParticleCommon.hlsli" 
#include "ShaderConstants.hlsli" 

//struct VertexIn
//{
//    float4 position : POSITION; 
//    float2 uv : TEXCOORD; 
//};

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

VertexOut main(uint vID : SV_VertexID, uint instID : SV_InstanceID)
{
    VertexOut vout;
    ParticleInstanceData inst = instanceBuffer[instID];

    // 板ポリゴンのUVとローカル座標を生成
    float2 uvList[6] =
    {
        float2(0, 1), // 左下 
        float2(1, 1), // 右下
        float2(0, 0), // 左上 
        float2(1, 1), // 右下 
        float2(1, 0), // 右上 
        float2(0, 0) // 左上
    };

    float2 uv = uvList[vID];
    // UVからローカル座標へ変換
    float2 localPos = float2(uv.x - 0.5f, 0.5f - uv.y);

    float3 worldPos;

    // Z軸回転の計算
    float cosR = cos(inst.rotationZ);
    float sinR = sin(inst.rotationZ);
    float rotatedX = localPos.x * cosR - localPos.y * sinR;
    float rotatedY = localPos.x * sinR + localPos.y * cosR;

    if (inst.isBillboard == 1)
    {
        float3 right = gFrameData.cameraRight;
        float3 up = gFrameData.cameraUp;
        worldPos = inst.position + (rotatedX * right * inst.scale.x) + (rotatedY * up * inst.scale.y);
    }
    else
    {
        worldPos = inst.position + float3(rotatedX * inst.scale.x, rotatedY * inst.scale.y, 0.0f);
    }

    vout.svpos = mul(float4(worldPos, 1.0f), gFrameData.viewProjectionMatrix);
    vout.uv = uv; // 計算したUVを渡す
    vout.color = inst.color;
    vout.color.rgb *= inst.intensity;
    vout.textureIndex = inst.textureIndex;

    return vout;
}