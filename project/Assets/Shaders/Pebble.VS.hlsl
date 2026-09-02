#include "ShaderConstants.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);

// NOTE: ComputeShaderでカリング・トランスフォーム計算済みのバッファを直接バインドする
StructuredBuffer<PebbleInstanceData> gInstanceData : register(t10);

struct PebbleVSInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
    float3 tangent : TANGENT0;
    uint instanceID : SV_InstanceID;
};

struct PebbleVSOutput
{
    float4 position : SV_POSITION;
    float3 worldPosition : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
    float3 tangent : TANGENT0;
    float2 velocity : TEXCOORD1;
    nointerpolation float3 colorVariation : COLOR0;
    float heightFactor : TEXCOORD2;
};

// ハミルトン積に基づくクォータニオン回転の最適化実装
float3 RotateVectorByQuat(float3 v, float4 q)
{
    float3 t = 2.0f * cross(q.xyz, v);
    return v + q.w * t + cross(q.xyz, t);
}

PebbleVSOutput main(PebbleVSInput input)
{
    PebbleVSOutput output;
    
    PebbleInstanceData instance = gInstanceData[input.instanceID];
    float3 pos = instance.posAndScale.xyz;
    float baseScale = instance.posAndScale.w;
    float4 quat = instance.rotationQuat;
    
    float3 anisoScale = instance.anisoAndEmbed.xyz;
    float embedRatio = instance.anisoAndEmbed.w;

    float3 localPos = input.position.xyz;

    // NOTE: メッシュのローカルY座標が [-0.5, 0.5] の範囲でモデリングされている前提。
    // 0.0(底面) ~ 1.0(天頂) にマッピングし、PSでのプロシージャルなコケや汚れのブレンドウェイトに使用する
    output.heightFactor = saturate(localPos.y + 0.5f);

    localPos *= (baseScale * anisoScale);

    // 接地感を出すためのオフセット（斜面配置時の浮きを防止）
    float pebbleHeight = baseScale * anisoScale.y;
    localPos.y -= (embedRatio * pebbleHeight);

    float3 rotatedPos = RotateVectorByQuat(localPos, quat);
    float3 worldPos = rotatedPos + pos;

    output.worldPosition = worldPos;
    output.texcoord = input.texcoord;
    
    // 非等方スケール（XYZで異なる倍率）による法線・接線の歪みを補正
    // TODO: スケール値が極端に0に近づく場合のゼロ除算対策 (現状は max 0.001f でクリップして回避)
    float3 localNormal = normalize(input.normal / max(anisoScale, 0.001f));
    float3 localTangent = normalize(input.tangent / max(anisoScale, 0.001f));
    output.normal = normalize(RotateVectorByQuat(localNormal, quat));
    output.tangent = normalize(RotateVectorByQuat(localTangent, quat));

    // TAAおよびモーションブラー用のVelocity計算 (現在と前フレームのNDC空間の差分)
    float4 clipPos = mul(float4(worldPos, 1.0f), gFrameData.viewProjectionMatrix);
    float4 prevClipPos = mul(float4(worldPos, 1.0f), gFrameData.prevViewProj);
    
    output.position = clipPos;
    
    float2 currentNDC = clipPos.xy / clipPos.w;
    float2 prevNDC = prevClipPos.xy / prevClipPos.w;
    output.velocity = (currentNDC - prevNDC) * float2(0.5f, -0.5f);
    
    output.colorVariation = instance.colorVariation;

    return output;
}