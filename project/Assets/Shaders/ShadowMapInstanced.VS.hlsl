#include "Object3D.hlsli"
#include "ShaderConstants.hlsli" 

ConstantBuffer<FrameData> gFrameData : register(b0);
// ライト情報（今は配列先頭をバインドだが複数に対応したい）
ConstantBuffer<DirectionalLight> gLight : register(b1);
ConstantBuffer<MaterialData> gMaterial : register(b5);
ConstantBuffer<InstanceOffset> gInstanceOffset : register(b7);
ConstantBuffer<ShadowData> gShadowData : register(b8);
ConstantBuffer<CascadeConstant> gCascadeConstant : register(b9);
StructuredBuffer<Object3DInstanceData> gInstanceData : register(t10);

struct ShadowVSOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

ShadowVSOutput main(VertexShaderInput input, uint instanceID : SV_InstanceID)
{
    ShadowVSOutput output;
    
    // 自分のインスタンスデータを取得
    uint index = gInstanceOffset.gBaseInstanceIndex + instanceID;
    float4x4 worldMatrix = gInstanceData[index].World;

    float4 localPos = input.position;

    // 揺らす処理 (Bubble)
    if (gMaterial.isBubble != 0)
    {
        float time = gFrameData.gTime * gMaterial.wobbleSpeed;
        float wave = sin(time + localPos.y * 5.0f) +
                     cos(time + localPos.z * 5.0f) +
                     sin(time + localPos.x * 5.0f);
        localPos.xyz += input.normal * wave * gMaterial.wobbleAmplitude;
    }
    
    // 揺らす処理 (Tree)
    if (gMaterial.enableTreeWind != 0)
    {
        // 自身のワールド座標
        float3 windWorldPos = mul(localPos, worldMatrix).xyz;
        
        // 空間的なズレの計算
        float phaseOffset = (windWorldPos.x + windWorldPos.z) * gMaterial.treeWindSpatialScale;

        // 時間軸の計算
        float time = gFrameData.gTime * gMaterial.treeWindSpeed + phaseOffset;

        // 複雑な揺れの生成
        float waveX = sin(time) * cos(time * 0.45f + phaseOffset);
        float waveZ = cos(time * 0.75f) * sin(time * 0.25f + phaseOffset);
        
        // 揺れ始める高さのしきい値
        float thresholdHeight = gMaterial.treeWindThresholdHeight;
        
        // localPos.y からしきい値を引く
        float normalizedHeight = max(0.0f, (localPos.y - thresholdHeight) * gMaterial.treeWindHeightScale);
  
        // 最終的な座標オフセット
        localPos.x += waveX * gMaterial.treeWindAmplitude * normalizedHeight;
        localPos.z += waveZ * gMaterial.treeWindAmplitude * normalizedHeight;
    }

    // World行列を適用
    float4 worldPos = mul(localPos, worldMatrix);

    // ライトビュープロジェクションを適用
    output.position = mul(worldPos, gShadowData.cascadeLightViewProj[gCascadeConstant.cascadeIndex]);
    output.texcoord = input.texcoord;

    return output;
}