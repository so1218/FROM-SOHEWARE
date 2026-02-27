#include "Object3D.hlsli"
#include "ShaderConstants.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);
cbuffer DirectionalLights : register(b1)
{
    DirectionalLight gDirectionalLights[MAX_DIRECTIONAL_LIGHTS];
};
ConstantBuffer<MaterialData> gMaterial : register(b5);

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
    float3 normal : NORMAL;
    float3 worldPosition : WORLD_POSITION;
    float4 color : COLOR;
    float4 shadowCoord : SHADOW_COORD;
};


// シャドウ強度を計算
float CalculateShadow(float4 shadowCoord, float3 normal);

PixelShaderOutput main(PixelInput input)
{
    PixelShaderOutput output;

    // テクスチャサンプリング
    float4 textureColor = gTexture.Sample(gSampler, input.texcoord);

    // アルファテスト (ここで草の形を切り抜く)
    if (textureColor.a < 0.5f)
    {
        discard;
    }

    // ベースカラーの決定
    float3 baseColor = textureColor.rgb * gMaterial.color.rgb * input.color.rgb;

    float3 toEye = normalize(gFrameData.cameraWorldPosition - input.worldPosition);
    float3 lightDir = normalize(-gDirectionalLights[0].direction);
    float3 normal = normalize(input.normal);

    // kore影の計算 (Shadow)
    float shadowFactor = 1.0f;
    if (gMaterial.addShadow != 0)
    {
        shadowFactor = CalculateShadow(input.shadowCoord, normal);
    }

    // ライティング
    float NdotL = dot(normal, lightDir) * 0.5f + 0.5f;
    float3 diffuse = baseColor * gDirectionalLights[0].color.rgb * gDirectionalLights[0].intensity * NdotL * shadowFactor;

    // 透過光 (Translucency) - 逆光で光る
    float viewDotLight = saturate(dot(toEye, -lightDir));
    float3 translucency = baseColor * pow(viewDotLight, 3.0f) * gDirectionalLights[0].color.rgb * 0.4f * shadowFactor;

    float3 finalColor = diffuse + translucency + (baseColor * 0.2f); // 0.2fはAmbient

    // 根元を暗くする (Root AO)
    finalColor *= smoothstep(1.0f, 0.3f, input.texcoord.y);

    // インスタンスカラーを適用
    finalColor *= input.color.rgb;

    // G-Bufferへの出力
    output.color = float4(finalColor, 1.0f);
    output.normal = float4(normal, 1.0f);
    // 草はツヤツヤさせないのでラフネス高め(0.9)、メタルネスゼロ(0.0)に固定
    output.material = float4(0.0f, 0.9f, 0.0f, 1.0f);

    return output;
}

float CalculateShadow(float4 shadowCoord, float3 normal)
{
    float3 projCoords = shadowCoord.xyz / shadowCoord.w;

    // 法線ベースのバイアス
    float3 lightDir = normalize(-gDirectionalLights[0].direction);
    float biasScale = saturate(1.0f - dot(normal, lightDir));

    float depthBias = gMaterial.shadowBias;
    float normalBias = 0.002f * biasScale;

    // NDC→UV
    projCoords.x = projCoords.x * 0.5f + 0.5f;
    projCoords.y = -projCoords.y * 0.5f + 0.5f;

    // 法線オフセット
    projCoords.xy += normal.xy * normalBias;

    float currentDepth = projCoords.z - depthBias;

    // 範囲外
    if (projCoords.z < 0.0f || projCoords.z > 1.0f ||
        projCoords.x < 0.0f || projCoords.x > 1.0f ||
        projCoords.y < 0.0f || projCoords.y > 1.0f)
    {
        return 1.0f;
    }

    // PCF
    float2 texelSize = 1.0f / float2(2048.0f, 2048.0f);
    float softness = max(gMaterial.shadowSoftness, 1.0f);

    float shadow = 0.0f;
    [unroll]
    for (int i = 0; i < 16; ++i)
    {
        float2 offset = poissonDisk[i] * texelSize * softness;
        shadow += gShadowMap.SampleCmpLevelZero(
            gShadowSampler,
            projCoords.xy + offset,
            currentDepth
        );
    }

    // 平均化（0.0が完全な影、1.0が完全な光）
    float shadowVisibility = shadow * (1.0f / 16.0f);

    float densityLimit = min(gMaterial.shadowDensity, 0.99f);
    
    // densityLimitが高いほど、薄いグレーの影が黒(0.0)に変換され、影が太くくっきりする
    return smoothstep(densityLimit, 1.0f, shadowVisibility);
}
