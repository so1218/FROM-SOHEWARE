#include "Object3D.hlsli"
#include "ShaderConstants.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);

cbuffer DirectionalLights : register(b1)
{
    DirectionalLight gDirectionalLights[MAX_DIRECTIONAL_LIGHTS];
};

// t10 に設定したインスタンスデータ
StructuredBuffer<GrassInstanceData> gInstanceData : register(t10);

struct VertexInput
{
    float4 position : POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL;
    // SV_InstanceID で何番目の草かを取得
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

    // 1. インスタンスデータの取得
    GrassInstanceData instance = gInstanceData[input.instanceID];

    // 2. 基本のワールド座標計算
    float4 worldPos = mul(input.position, instance.world);

    // 3. 風の計算 (Wind Animation)
    // input.texcoord.y は下(根元)が1.0、上(先端)が0.0だと仮定
    // もしモデルのUVが逆なら (1.0 - input.texcoord.y) にしてください
    float windWeight = 1.0f - input.texcoord.y;
    
    // ワールド座標をシードにして揺れをズラす
    float windSpeed = gFrameData.gTime * 2.0f;
    float windPhase = worldPos.x * 0.5f + worldPos.z * 0.5f;
    
    // X軸とZ軸にSine波で揺れを加える
    float windSway = sin(windSpeed + windPhase) * 0.5f; // 揺れ幅
    
    // 根元は揺らさず、先端に行くほど揺らす
    worldPos.x += windSway * windWeight;
    worldPos.z += cos(windSpeed + windPhase) * 0.2f * windWeight;

    // 4. カメラプロジェクション
    output.position = mul(worldPos, gFrameData.viewProjectionMatrix);
    output.shadowCoord = mul(worldPos, gDirectionalLights[0].viewProj);
    output.worldPosition = worldPos.xyz;
    output.texcoord = input.texcoord;
    
    // 5. 法線のフェイク (プロのテクニック)
    // 板ポリの法線ではなく、やや上向き(Y=1)にブレンドすることで
    // ふんわりとしたアニメ調/自然なライティングになる
    float3 worldNormal = mul(input.normal, (float3x3) instance.world);
    worldNormal = normalize(lerp(worldNormal, float3(0.0f, 1.0f, 0.0f), 0.7f));
    output.normal = worldNormal;

    // インスタンスカラー（ランダムな色など）を渡す
    output.color = instance.color;

    return output;
}