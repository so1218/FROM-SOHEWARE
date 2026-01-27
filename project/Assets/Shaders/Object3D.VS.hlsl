#include "Object3D.hlsli"
#include "ShaderConstants.hlsli"

ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);

cbuffer DirectionalLights : register(b1)
{
    DirectionalLight gDirectionalLights[MAX_DIRECTIONAL_LIGHTS];
};

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    output.position = mul(input.position, gTransformationMatrix.WVP);
    output.texcoord = input.texcoord;
    output.normal = normalize(mul(input.normal, (float32_t3x3) gTransformationMatrix.WorldInverseTranspose));
     // ワールド空間での頂点位置を計算
    output.worldPosition = mul(input.position, gTransformationMatrix.World).xyz;
    
    float4 worldPos = float4(output.worldPosition, 1.0f);
    output.shadowCoord = mul(worldPos, gDirectionalLights[0].viewProj);
    // ワールド変換行列の回転成分だけを適用して渡す
    output.tangent = normalize(mul(input.tangent, (float3x3) gTransformationMatrix.World));
    
    output.worldColor = gTransformationMatrix.WorldColor;
    
    return output;
}
