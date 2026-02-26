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
