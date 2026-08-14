#include "ShaderConstants.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);

// Pebble専用のインスタンスデータ (CSが出力したバッファをそのまま受け取る)
StructuredBuffer<PebbleInstanceData> gInstanceData : register(t10);

struct PebbleVSInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
    float3 tangent : TANGENT0;
    uint instanceID : SV_InstanceID;
};

struct VertexShaderOutput
{
    float4 position : SV_POSITION; // クリップ空間座標 (ラスタライザ用)
    float3 worldPosition : POSITION0; // ワールド空間位置 (ライティング/距離フォグ用)
    float2 texcoord : TEXCOORD0; // UV座標
    float3 normal : NORMAL0; // ワールド空間法線
    float3 tangent : TANGENT0; // ワールド空間接線
    float2 velocity : TEXCOORD1; // TAA/モーションブラー用 Velocity (VSで事前計算)
    nointerpolation float3 colorVariation : COLOR0;
    float heightFactor : TEXCOORD2;
};

// ==========================================
// クォータニオンによるベクトル回転関数 (高速版)
// ==========================================
float3 RotateVectorByQuat(float3 v, float4 q)
{
    float3 t = 2.0f * cross(q.xyz, v);
    return v + q.w * t + cross(q.xyz, t);
}

VertexShaderOutput main(PebbleVSInput input)
{
    VertexShaderOutput output;
    
    PebbleInstanceData instance = gInstanceData[input.instanceID];
    float3 pos = instance.posAndScale.xyz;
    float baseScale = instance.posAndScale.w;
    float4 quat = instance.rotationQuat;
    
    // anisoAndEmbed: xyz = 非等方スケール, w = embedRatio (埋め込み率)
    float3 anisoScale = instance.anisoAndEmbed.xyz;
    float embedRatio = instance.anisoAndEmbed.w;

    float3 localPos = input.position.xyz;

    // ★ 1. 高さ割合の計算 (0.0 = 底面, 1.0 = 天頂)
    // メッシュの原点が中央(Y=0)にあリ、-0.5~0.5 の範囲と仮定
    output.heightFactor = saturate(localPos.y + 0.5f);

    // ★ 2. スケール適用
    localPos *= (baseScale * anisoScale);

    // ★ 3. 埋め込み処理 (embedRatio に応じて小石の高さをY軸マイナス方向へ押し込む)
    float pebbleHeight = baseScale * anisoScale.y;
    localPos.y -= (embedRatio * pebbleHeight);

    // 回転・ワールド移動
    float3 rotatedPos = RotateVectorByQuat(localPos, quat);
    float3 worldPos = rotatedPos + pos;

    output.worldPosition = worldPos;
    output.texcoord = input.texcoord;
    
    // 法線・接線の計算
    float3 localNormal = normalize(input.normal / max(anisoScale, 0.001f));
    float3 localTangent = normalize(input.tangent / max(anisoScale, 0.001f));
    output.normal = normalize(RotateVectorByQuat(localNormal, quat));
    output.tangent = normalize(RotateVectorByQuat(localTangent, quat));

    // クリップ座標変換
    float4 clipPos = mul(float4(worldPos, 1.0f), gFrameData.viewProjectionMatrix);
    float4 prevClipPos = mul(float4(worldPos, 1.0f), gFrameData.prevViewProj);
    output.position = clipPos;
    float2 currentNDC = clipPos.xy / clipPos.w;
    float2 prevNDC = prevClipPos.xy / prevClipPos.w;
    output.velocity = (currentNDC - prevNDC) * float2(0.5f, -0.5f);
    
    output.colorVariation = instance.colorVariation;

    return output;
}