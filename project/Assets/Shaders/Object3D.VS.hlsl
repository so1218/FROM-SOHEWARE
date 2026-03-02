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
        // 自身のワールド座標
        float3 worldPos = mul(localPos, gTransformationMatrix.World).xyz;
        
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
