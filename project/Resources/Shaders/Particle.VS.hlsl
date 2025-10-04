#include "ParticleCommon.hlsli" 
// 頂点入力：板ポリ（1インスタンスあたり4頂点）
struct VertexIn
{
    float4 position : POSITION; // ローカル座標（-0.5〜+0.5）
    float2 uv : TEXCOORD; // UV
};

// Camera用定数バッファ
cbuffer CameraBuffer : register(b0)
{
    float4x4 viewProjection;
    float3 cameraRight; // X軸方向
    float padding0; // パディング
    float3 cameraUp; // Y軸方向
};

// GPUインスタンシング用バッファ
// インスタンシング用のデータを格納するためのバッファ
StructuredBuffer<InstanceData> instanceBuffer : register(t0);

// 出力
struct VertexOut
{
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD;
    float4 color : COLOR;
    float textureIndex : TEXCOORD1;
};

// メインシェーダー
VertexOut main(VertexIn vin, uint instanceId : SV_InstanceID)
{
    VertexOut vout;

    // インスタンスデータ
    InstanceData inst = instanceBuffer[instanceId];
    float4x4 world = inst.world;

    // 中心位置（移動成分）
    float3 center = world[3].xyz;

    // スケールを world の x/y 軸から抽出
    float scaleX = length(world[0].xyz);
    float scaleY = length(world[1].xyz);

    // カメラ方向のビルボードベクトル
    float3 right = cameraRight;
    float3 up = cameraUp;

    // Z軸回転（ラジアン）を使ってローカルXYを回転
    float cosR = cos(inst.rotationZ);
    float sinR = sin(inst.rotationZ);
    float rotatedX = vin.position.x * cosR - vin.position.y * sinR;
    float rotatedY = vin.position.x * sinR + vin.position.y * cosR;

    // カメラ方向ベースに、スケール＆回転済みのローカルオフセットを加える
    float3 worldPos = center
        + rotatedX * right * scaleX
        + rotatedY * up * scaleY;

    // ワールド→クリップ座標へ
    vout.svpos = mul(float4(worldPos, 1.0f), viewProjection);

    // その他属性
    vout.uv = vin.uv;
    vout.color = inst.color;
    vout.textureIndex = inst.textureIndex;

    return vout;
}
