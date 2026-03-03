#include "Object3D.hlsli"
#include "ShaderConstants.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);

cbuffer DirectionalLights : register(b1)
{
    DirectionalLight gDirectionalLights[MAX_DIRECTIONAL_LIGHTS];
};

ConstantBuffer<MaterialData> gMaterial : register(b5);

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
    
    // ✨ この頂点が属するインスタンスのデータを取得
    Object3DInstanceData instance = gInstanceData[input.instanceID];
    
    float4 localPos = input.position;

    // --- (バブルや木の揺れ処理はそのまま) ---
    // ※ 揺れ計算内の gTransformationMatrix.World は instance.World に変えます
    // ----------------------------------------
    
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
        float3 worldPos = mul(localPos, instance.World).xyz;
        
        // 空間的なズレの計算
        float phaseOffset = (worldPos.x + worldPos.z) * gMaterial.treeWindSpatialScale;

        // 時間軸の計算
        float time = gFrameData.gTime * gMaterial.treeWindSpeed + phaseOffset;

        // 複雑な揺れの生成
        float waveX = sin(time) * cos(time * 0.45f + phaseOffset);
        float waveZ = cos(time * 0.75f) * sin(time * 0.25f + phaseOffset);

        // 高さによるウェイト
        float heightWeight = max(0.0f, localPos.y * gMaterial.treeWindHeightScale);
        
        // 最終的な座標オフセット
        localPos.x += waveX * gMaterial.treeWindAmplitude * heightWeight;
        localPos.z += waveZ * gMaterial.treeWindAmplitude * heightWeight;
    }

    // ✨ WVP行列は C++ から送るのをやめ、シェーダー内で「World × ViewProj」として計算します
    float4 worldPos = mul(localPos, instance.World);
    output.worldPosition = worldPos.xyz;
    
    // ViewProjectionMatrix は gFrameData にあると仮定（なければ定数バッファで送る必要があります）
    output.position = mul(worldPos, gFrameData.viewProjectionMatrix);

    output.texcoord = input.texcoord;
    
    // ✨法線の変換もインスタンスごとの行列を使う
    output.normal = normalize(mul(input.normal, (float32_t3x3) instance.WorldInverseTranspose));
    output.tangent = normalize(mul(input.tangent, (float3x3) instance.World));
    
    output.shadowCoord = mul(worldPos, gDirectionalLights[0].viewProj);
    output.worldColor = instance.WorldColor; // ✨ インスタンスごとの色
    
    return output;
}