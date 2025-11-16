#include "ParticleCommon.hlsli" 
#include "ShaderConstants.hlsli" 
// 頂点入力：板ポリ（1インスタンスあたり4頂点）
struct VertexIn
{
    float4 position : POSITION; // ローカル座標（-0.5〜+0.5）
    float2 uv : TEXCOORD; // UV
};

// GPUインスタンシング用バッファ
// インスタンシング用のデータを格納するためのバッファ
StructuredBuffer<ParticleInstanceData> instanceBuffer : register(t0);
ConstantBuffer<FrameData> gFrameData : register(b0);

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
    ParticleInstanceData inst = instanceBuffer[instanceId];
    float4x4 world = inst.worldMatrix;
    float3 worldPos;
    
    if (inst.isBillboard == 1)
    {
        // 中心位置（移動成分）
        float3 center = world[3].xyz;

        // スケールを world の x/y 軸から抽出
        float scaleX = length(world[0].xyz);
        float scaleY = length(world[1].xyz);

        // カメラ方向のビルボードベクトル
        float3 right = gFrameData.cameraRight;
        float3 up = gFrameData.cameraUp;

        // Z軸回転（ラジアン）を使ってローカルXYを回転
        float cosR = cos(inst.rotationZ);
        float sinR = sin(inst.rotationZ);
        float rotatedX = vin.position.x * cosR - vin.position.y * sinR;
        float rotatedY = vin.position.x * sinR + vin.position.y * cosR;

        // カメラ方向ベースに、スケール＆回転済みのローカルオフセットを加える
        worldPos = center
        + rotatedX * right * scaleX
        + rotatedY * up * scaleY;
    }
    else
    {
        // ビルボードが無効な場合の処理
        worldPos = mul(vin.position, world).xyz;
    }

    // ワールド→クリップ座標へ
    vout.svpos = mul(float4(worldPos, 1.0f), gFrameData.viewProjectionMatrix);

    // その他属性
    vout.uv = vin.uv;
    vout.color = inst.color;
    vout.textureIndex = inst.textureIndex;

    return vout;
}
