前回までの最適化の文脈を引き継ぎ、 不要な装飾やチュートリアル感を排除しました。

「
なぜその数式や近似を使っているのか」「 メモリ帯域や処理負荷をどうやって抑えているのか」
という、 実務のグラフィックスプログラマが最も気にするポイントに絞ってコメントを記述しています。

High-
level shaderlanguage
#include "Object3D.hlsli"
#include "ShaderConstants.hlsli"
#include "ShadowUtils.hlsli"
#include "LightingUtils.hlsli"
#include "NormalUtils.hlsli"
#include "PBRUtils.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);

cbuffer DirectionalLights : register(b1)
{
    DirectionalLight gDirectionalLights[MAX_DIRECTIONAL_LIGHTS];
};

cbuffer PointLights : register(b2)
{
    PointLight gPointLights[MAX_POINT_LIGHTS];
};

cbuffer SpotLights : register(b3)
{
    SpotLight gSpotLights[MAX_SPOT_LIGHTS];
};

ConstantBuffer<GlobalEnvironmentData> gEnvironmentData : register(b4);
ConstantBuffer<TrunkMaterialData> gMaterial : register(b6);
ConstantBuffer<ShadowData> gShadowData : register(b8);

Texture2D<float4> gTexture : register(t0);
Texture2DArray<float> gShadowMapArray : register(t2);
Texture2D<float4> gToonRamp : register(t3);
Texture2D<float4> gNormalTexture : register(t5);

SamplerState gSampler : register(s0);
SamplerComparisonState gShadowSampler : register(s1);
SamplerState gClampSampler : register(s2);

struct PixelInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 worldPosition : WORLD_POSITION;
    float4 color : COLOR0;
    float3 instanceTint : COLOR1;
    float lodFade : BLENDWEIGHT;
};

// TAAと相性の良い Interleaved Gradient Noise (IGN)
float InterleavedGradientNoise(float2 pixelPos)
{
    float3 magic = float3(0.06711056f, 0.00583715f, 52.9829189f);
    return frac(magic.z * frac(dot(pixelPos, magic.xy)));
}

PixelShaderOutput main(PixelInput input)
{
    PixelShaderOutput output;
    
    // -------------------------------------------------------------------------
    // LOD遷移
    // -------------------------------------------------------------------------
    // 半透明ブレンドによるオーバードローを避けるためディザリングによるクリップを使用
    float dither = InterleavedGradientNoise(input.position.xy);
    clip(input.lodFade - dither);

    // -------------------------------------------------------------------------
    // 材質特性と天候パラメーターの連動
    // -------------------------------------------------------------------------
    float4 textureColor = gTexture.Sample(gSampler, input.texcoord);
    float3 baseColor = textureColor.rgb * input.instanceTint * gMaterial.color.rgb * max(gMaterial.albedoMultiplier, 0.0f);

    float wetness = gEnvironmentData.wetness;
    
    // 樹皮の光学特性を近似
    // 水分を含むことによる光の内部散乱でアルベドを暗く落とし、表面の水膜を表現するためラフネスを低下
    baseColor = lerp(baseColor, baseColor * 0.55f, wetness);
    
    float baseRoughness = clamp(gMaterial.roughness, 0.05f, 1.0f);
    float roughness = lerp(baseRoughness, 0.2f, wetness);

    // -------------------------------------------------------------------------
    // 法線構築
    // -------------------------------------------------------------------------
    // 頂点レイアウトのサイズ削減のため、Bitangentは頂点ストリームに含めず、
    // ピクセルシェーダー内でNormalとTangentの外積から動的に復元
    float3 worldNormal = normalize(input.normal);
    float3 worldTangent = normalize(input.tangent);
    float3 worldBitangent = cross(worldNormal, worldTangent);

    float3 normal = worldNormal;
    if (gMaterial.enableNormalMap != 0)
    {
        normal = CalculateNormalFromMap(worldNormal, worldTangent, input.texcoord, gMaterial.normalIntensity, gNormalTexture, gSampler);
    }

    float3 toEyeWorld = normalize(gFrameData.cameraWorldPosition - input.worldPosition);

    float shadowFactor = 1.0f;
    if (gDirectionalLights[0].enable && gMaterial.addShadow != 0)
    {
        float viewDepth = distance(gFrameData.cameraWorldPosition, input.worldPosition);
        float3 lightDir = normalize(-gDirectionalLights[0].direction);
        shadowFactor = CalculateShadowCSM(
            input.worldPosition, normal, viewDepth, lightDir,
            gMaterial.shadowDensity, gShadowData.cascadeSplits,
            gMaterial.shadowNormalBias, gMaterial.shadowBias, gMaterial.shadowSoftness,
            gShadowData.cascadeLightViewProj, gShadowMapArray, gShadowSampler);
    }

    SurfaceData surface;
    surface.albedo = baseColor;
    
    // sRGB -> Linear変換における pow(x, 2.2) の計算負荷を避けるための高速な近似
    surface.pbrAlbedo = baseColor * baseColor;
    
    surface.specularColor = gMaterial.specularColor.rgb;
    surface.normal = normal;
    surface.roughness = roughness;
    surface.metalness = saturate(gMaterial.metalness);
    surface.shininess = gMaterial.shininess;
    surface.diffuseReflection = gMaterial.diffuseReflection;
    surface.lightMode = gMaterial.lightMode;

    // -------------------------------------------------------------------------
    // ライティング & IBL近似
    // -------------------------------------------------------------------------
    float3 finalColor = 0.0f.xxx;
    if (gMaterial.enableLighting != 0)
    {
        finalColor += ApplyDirectionalLights(surface, toEyeWorld, shadowFactor, gDirectionalLights, gToonRamp, gClampSampler);
        finalColor += ApplyPointLights(surface, input.worldPosition, toEyeWorld, gPointLights);
        finalColor += ApplySpotLights(surface, input.worldPosition, toEyeWorld, gSpotLights);

        if (gMaterial.lightMode == SHADING_MODEL_PBR)
        {
            float NdotV = max(dot(surface.normal, toEyeWorld), 0.0f);
            
            // F0の計算。非金属のデフォルト値(0.04)に対し、濡れ度合いに応じて水の屈折率(0.02)へと遷移
            float3 baseF0 = lerp(0.04f.xxx, surface.pbrAlbedo, surface.metalness);
            float3 F0 = lerp(baseF0, 0.02f.xxx, wetness);
            
            float3 kS = F_SchlickRoughness(NdotV, F0, surface.roughness);
            float3 kD = (1.0f.xxx - kS) * (1.0f - surface.metalness);

            float combinedAO = lerp(gMaterial.shadowEnvStrength, 1.0f, shadowFactor) * input.color.a;
            
            // 空色と地面色を用いた半球ライティングにより、IBLを近似
            float3 reflectDir = reflect(-toEyeWorld, surface.normal);
            float skyWeight = saturate(reflectDir.y * 0.5f + 0.5f);
            float3 envSkyColor = lerp(gEnvironmentData.groundColor.rgb, gEnvironmentData.skyColor.rgb, skyWeight);

            float3 ambientSpecular = envSkyColor * kS;

            float skyLight = saturate(surface.normal.y * 0.5f + 0.5f);
            float3 ambientDiffuse = kD * surface.pbrAlbedo * lerp(gEnvironmentData.groundColor.rgb, gEnvironmentData.skyColor.rgb, skyLight);

            float3 ambient = (ambientDiffuse + ambientSpecular) * gMaterial.environmentMapIntensity;
            
            finalColor += ambient * combinedAO;
        }
    }
    else
    {
        finalColor = surface.albedo;
    }

    output.color = float4(finalColor, 1.0f);
    output.normal = float4(normal, 1.0f);
    output.material = float4(surface.metalness, surface.roughness, 0.0f, 1.0f);
    
    // TODO: Trunk(幹)のVelocityは静的オブジェクトとして0を出力しているが、
    // 今後ワールド全体の風による GlobalWind を適用する場合は要修正。
    output.velocity = float2(0.0f, 0.0f);
    
    return output;
}