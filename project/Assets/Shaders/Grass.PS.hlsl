#include "Object3D.hlsli"
#include "ShaderConstants.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);
cbuffer DirectionalLights : register(b1)
{
    DirectionalLight gDirectionalLights[MAX_DIRECTIONAL_LIGHTS];
};
ConstantBuffer<MaterialData> gMaterial : register(b5);

Texture2D<float4> gTexture : register(t0);
Texture2D<float> gShadowMap : register(t2);
SamplerState gSampler : register(s0);
SamplerComparisonState gShadowSampler : register(s1);

// 影計算
float CalculateShadow(float4 shadowCoord, float3 normal)
{
    float3 projCoords = shadowCoord.xyz / shadowCoord.w;
    projCoords.xy = projCoords.xy * 0.5f + 0.5f;
    projCoords.y = 1.0f - projCoords.y;
    
    if (projCoords.z > 1.0f || projCoords.x < 0.0f || projCoords.x > 1.0f || projCoords.y < 0.0f || projCoords.y > 1.0f)
        return 1.0f;

    // 0.005fはバイアス
    return gShadowMap.SampleCmpLevelZero(gShadowSampler, projCoords.xy, projCoords.z - 0.005f);
}

PixelShaderOutput main(PixelShaderInput input)
{
    PixelShaderOutput output;

    // テクスチャ
    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);

    // アルファテスト (0.5は調整)
    if (textureColor.a < 0.5f)
        discard;

    // ライティング準備
    float3 normal = normalize(input.normal);
    float3 lightDir = normalize(-gDirectionalLights[0].direction);
    
    // Half-Lambert (板ポリの裏側も明るく)
    float dotNL = dot(normal, lightDir);
    float halfLambert = pow(dotNL * 0.5f + 0.5f, 2.0f);

    // 影
    float shadowFactor = 1.0f;
    if (gDirectionalLights[0].enable && gMaterial.addShadow != 0)
    {
        shadowFactor = CalculateShadow(input.shadowCoord, normal);
    }

    // 最終カラー
    float3 baseColor = textureColor.rgb * input.worldColor.rgb * gMaterial.color.rgb;
    float3 diffuse = gDirectionalLights[0].color.rgb * gDirectionalLights[0].intensity * halfLambert * shadowFactor;

    output.color = float4(baseColor * diffuse, 1.0f);
    output.normal = float4(normal, 1.0f);
    output.material = float4(gMaterial.metalness, gMaterial.roughness, 0.0f, 1.0f);

    return output;
}