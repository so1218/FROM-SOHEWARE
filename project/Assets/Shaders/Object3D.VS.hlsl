#include "Object3D.hlsli"
#include "ShaderConstants.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);

cbuffer DirectionalLights : register(b1)
{
    DirectionalLight gDirectionalLights[MAX_DIRECTIONAL_LIGHTS];
};

ConstantBuffer<MaterialData> gMaterial : register(b5);
ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b6);

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    
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
        // 自身のワールド座標を取得
        float3 worldPos = mul(localPos, gTransformationMatrix.World).xyz;
        
        // 1. 空間的なズレ（位相）の計算
        // treeWindSpatialScale を掛けることで、バラつき具合を調整可能に
        float phaseOffset = (worldPos.x + worldPos.z) * gMaterial.treeWindSpatialScale;

        // 2. 時間軸の計算
        float time = gFrameData.gTime * gMaterial.treeWindSpeed + phaseOffset;

        // 3. 複雑な揺れの生成
        // 0.5f や 0.8f などの係数も variation パラメータで少し変化させるとより自然になります
        float waveX = sin(time) * cos(time * 0.45f + phaseOffset);
        float waveZ = cos(time * 0.75f) * sin(time * 0.25f + phaseOffset);

        // 4. 高さによるウェイト（根本を固定）
        // localPos.y に treeWindHeightScale を掛けて調整
        // max(0, ...) で地面より下（もしあれば）が逆に揺れるのを防ぐ
        float heightWeight = max(0.0f, localPos.y * gMaterial.treeWindHeightScale);
        
        // 5. 最終的な座標オフセット
        // heightWeight が 0 なら全く動かず、高いほど Amplitude の影響を強く受ける
        localPos.x += waveX * gMaterial.treeWindAmplitude * heightWeight;
        localPos.z += waveZ * gMaterial.treeWindAmplitude * heightWeight;
    }

    output.position = mul(localPos, gTransformationMatrix.WVP);
    output.texcoord = input.texcoord;
    output.normal = normalize(mul(input.normal, (float32_t3x3) gTransformationMatrix.WorldInverseTranspose));
    output.worldPosition = mul(localPos, gTransformationMatrix.World).xyz;
    
    float4 worldPos = float4(output.worldPosition, 1.0f);
    output.shadowCoord = mul(worldPos, gDirectionalLights[0].viewProj);
    output.tangent = normalize(mul(input.tangent, (float3x3) gTransformationMatrix.World));
    output.worldColor = gTransformationMatrix.WorldColor;
    
    return output;
}
