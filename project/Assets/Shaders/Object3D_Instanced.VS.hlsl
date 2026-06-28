#include "Object3D.hlsli"
#include "ShaderConstants.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);

cbuffer DirectionalLights : register(b1)
{
    DirectionalLight gDirectionalLights[MAX_DIRECTIONAL_LIGHTS];
};

ConstantBuffer<MaterialData> gMaterial : register(b5);
ConstantBuffer<InstanceOffset> gInstanceOffset : register(b7);

StructuredBuffer<Object3DInstanceData> gInstanceData : register(t10);

struct Object3DVSInputInstanced
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
    float3 tangent : TANGENT0;
    float3 smoothNormal : TEXCOORD1;
    uint instanceID : SV_InstanceID;
};

VertexShaderOutput main(Object3DVSInputInstanced input)
{
    VertexShaderOutput output;
    
    // インスタンスのデータを取得(SV_InstanceIDにオフセットを足して正しいデータを取り出す)
    uint index = input.instanceID + gInstanceOffset.gBaseInstanceIndex;
    Object3DInstanceData instance = gInstanceData[index];
    
    float4 localPos = input.position;

    // バブルの揺れ処理
    if (gMaterial.isBubble != 0)
    {
        float time = gFrameData.gTime * gMaterial.wobbleSpeed;
        float wave = sin(time + localPos.y * 5.0f) +
                     cos(time + localPos.z * 5.0f) +
                     sin(time + localPos.x * 5.0f);
        
        // 法線方向に押し出す
        localPos.xyz += input.normal * wave * gMaterial.wobbleAmplitude;
    }
    
    if (gMaterial.enableTreeWind != 0)
    {
        // 自身のワールド座標
        float3 windWorldPos = mul(localPos, instance.World).xyz;
        
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

   // WVPは送らず、揺れ計算後のlocalPosを使って画面座標まで変換
    float4 worldPos = mul(localPos, instance.World);
    output.worldPosition = worldPos.xyz;
    
    // ViewProjectionMatrixを掛けて最終的な画面上の座標へ
    output.position = mul(worldPos, gFrameData.viewProjectionMatrix);

    output.texcoord = input.texcoord;
    
    // 法線の変換
    output.normal = normalize(mul(input.normal, (float32_t3x3) instance.WorldInverseTranspose));
    output.tangent = normalize(mul(input.tangent, (float3x3) instance.World));
    
    output.shadowCoord = mul(worldPos, gDirectionalLights[0].viewProj);
    output.worldColor = instance.WorldColor;
    
    return output;
}