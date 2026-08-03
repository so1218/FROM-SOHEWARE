#include "Object3D.hlsli"
#include "ShaderConstants.hlsli"
#include "GridUtils.hlsli"
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
cbuffer AreaLightsBuffer : register(b4)
{
    AreaLight gAreaLights[MAX_AREA_LIGHTS];
};
ConstantBuffer<MaterialData> gMaterial : register(b5);
ConstantBuffer<ShadowData> gShadowData : register(b8);

Texture2D<float4> gTexture : register(t0);
TextureCube<float4> gEnvironmentTexture : register(t1);
Texture2DArray<float> gShadowMapArray : register(t2);
Texture2D<float4> gToonRamp : register(t3);
Texture2D<float4> gDissolveTexture : register(t4);
Texture2D<float4> gNormalTexture : register(t5);
Texture2D<float4> gRippleTexture : register(t6);
Texture2D<float> gPuddleNoiseTexture : register(t7);
Texture2D<float> gPOMHeightMap : register(t8);

SamplerState gSampler : register(s0);
SamplerComparisonState gShadowSampler : register(s1);
SamplerState gClampSampler : register(s2);

PixelShaderOutput main(PixelShaderInput input)
{
    PixelShaderOutput output;
    
    float3 toEyeWorld = normalize(gFrameData.cameraWorldPosition - input.worldPosition);
    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float2 finalUV = transformedUV.xy;
    float3 normalizedInputNormal = normalize(input.normal);
    
    // POM
    float pomSelfShadow = 1.0f;
    float2 pomUV = finalUV * gMaterial.normalTiling;
    
    // 負荷対策: Triplanar時の3軸サンプリングとPOMの併用は極端に重いため排他制御
    if (gMaterial.enablePOM != 0 && gMaterial.useTriplanar == 0)
    {
        float3 N = normalizedInputNormal;
        float3 T = normalize(input.tangent);
        float3 B = normalize(cross(N, T));
        float3x3 TBN = float3x3(T, B, N);
        float3 toEyeTS = mul(TBN, toEyeWorld);
        
        // SampleGrad用：pomUVを使って計算する
        float2 dx = ddx(pomUV);
        float2 dy = ddy(pomUV);

        float parallaxHeight = 0.0f;
        // POMの計算には pomUV を渡す
        float2 newPomUV = CalculateParallaxOcclusionMapping(pomUV, toEyeTS, dx, dy, gMaterial.pomHeightScale, gMaterial.pomMaxSteps, gMaterial.pomMinSteps,
            gPOMHeightMap, gSampler,
            parallaxHeight);

        // POMによってズレた移動量を計算
        float2 pomOffset = newPomUV - pomUV;
        pomUV = newPomUV;

        // アルベド用のUV(finalUV)にも、スケールを補正してズレを適用
        float2 safeTiling = max(gMaterial.normalTiling, 0.0001f);
        finalUV += pomOffset / safeTiling;

        // POMセルフシャドウ (メインライトのみ適用)
        if (gDirectionalLights[0].enable != 0)
        {
            float3 lightDirTS = mul(TBN, normalize(-gDirectionalLights[0].direction));
            // シャドウ計算にも pomUV を渡す
            pomSelfShadow = CalculatePOMSoftShadow(lightDirTS, pomUV, parallaxHeight, dx, dy,
                gMaterial.pomHeightScale, gMaterial.pomMaxSteps, gMaterial.pomMinSteps,
                gPOMHeightMap, gSampler);
        }
    }
    
    // --------------------------------------------------------
    // Albedo & Alpha Test
    // --------------------------------------------------------
    float3 worldNormal = normalizedInputNormal;
    float4 textureColor;
    float blendSharpness = gMaterial.triplanarBlendSharpness > 0.0f ? gMaterial.triplanarBlendSharpness : 4.0f;

    if (gMaterial.useTriplanar != 0)
    {
        textureColor = CalculateTriplanarColor(input.worldPosition, worldNormal, gMaterial.triplanarScale, blendSharpness,
            gTexture, gSampler);
    }
    else
    {
        textureColor = gTexture.Sample(gSampler, finalUV);
    }
        
    // 早期カリング
    if (textureColor.a * gMaterial.color.a <= gMaterial.alphaTestThreshold)
        discard;
    
    float3 baseColor = textureColor.rgb;

    // --------------------------------------------------------
    // Dissolve エフェクト
    // --------------------------------------------------------
    float3 dissolveEdgeEmission = 0.0f.xxx;
    if (gMaterial.enableDissolve != 0)
    {
        // TODO: 負荷が高ければ頂点シェーダ側でのサンプリングに逃がすか検討
        float noiseValue = gDissolveTexture.Sample(gSampler, finalUV).r;
        if (noiseValue <= gMaterial.dissolveThreshold)
            discard;

        float difference = noiseValue - gMaterial.dissolveThreshold;
        if (difference < gMaterial.edgeWidth)
        {
            float t = smoothstep(0.0f, 1.0f, 1.0f - (difference / gMaterial.edgeWidth));
            dissolveEdgeEmission = gMaterial.edgeColor * t * gMaterial.edgeIntensity;
        }
    }

    // --------------------------------------------------------
    // Debug: Art Grid
    // --------------------------------------------------------
    if (gMaterial.isArtGrid)
    {
        if (ShouldDiscardArtGrid(input.texcoord))
            discard;

        output.color = float4(DrawArtGridColor(input.texcoord, gFrameData.iResolution), 1.0f);
        
        // G-Bufferの破綻防止
        output.normal = float4(0.0f, 1.0f, 0.0f, 1.0f);
        output.material = float4(0.0f, 1.0f, 0.0f, 1.0f);
        return output;
    }
    
    // --------------------------------------------------------
    // Shadow & Normal
    // --------------------------------------------------------
    float shadowFactor = 1.0f;
    if (gDirectionalLights[0].enable && gMaterial.addShadow != 0)
    {
        float viewDepth = distance(gFrameData.cameraWorldPosition, input.worldPosition);
        float3 lightDir = normalize(-gDirectionalLights[0].direction);
        shadowFactor = CalculateShadowCSM(input.worldPosition, worldNormal, viewDepth, lightDir,
            gMaterial.shadowDensity, gShadowData.cascadeSplits,
            gMaterial.shadowNormalBias, gMaterial.shadowBias, gMaterial.shadowSoftness,
            gShadowData.cascadeLightViewProj, 
            gShadowMapArray, 
            gShadowSampler);
        shadowFactor = min(shadowFactor, pomSelfShadow); // CSMとPOM影の暗い方を採用
    }
    
    float3 normal = worldNormal;
    if (gMaterial.enableNormalMap != 0)
    {
        if (gMaterial.useTriplanar != 0)
        {
            float3 triNormal = CalculateTriplanarNormal(input.worldPosition, worldNormal, gMaterial.triplanarScale, blendSharpness,
                gNormalTexture, gSampler);
            normal = normalize(lerp(worldNormal, triNormal, gMaterial.normalIntensity));
        }
        else
        {
            normal = CalculateNormalFromMap(worldNormal, input.tangent, pomUV, gMaterial.normalIntensity, gNormalTexture, gSampler);
        }
    }
    
    float currentRoughness = clamp(gMaterial.roughness, 0.05f, 1.0f);
    float currentMetalness = saturate(gMaterial.metalness);
    
    // --------------------------------------------------------
    // Wetness & Ripple (雨・水たまり表現)
    // --------------------------------------------------------
    float3 addedPuddleEmission = 0.0f.xxx;
    if (gMaterial.enableRipple != 0 && gMaterial.wetness > 0.0f)
    {
        float globalWetness = gMaterial.wetness;
        float puddleDepth = 0.0f;

        if (gMaterial.usePuddle != 0)
        {
            float2 puddleUV = input.worldPosition.xz * gMaterial.puddleScale;
            float noiseVal = gPuddleNoiseTexture.Sample(gSampler, puddleUV).r;
            float edgeSoftness = max(gMaterial.puddleFalloff, 0.001f);
            puddleDepth = smoothstep(gMaterial.wetness, gMaterial.wetness - edgeSoftness, noiseVal);
        }

        float effectiveWetness = max(globalWetness, puddleDepth);
        // 水濡れによるラフネスの平滑化
        currentRoughness = lerp(currentRoughness, 0.01f, effectiveWetness);

        if (gMaterial.usePuddle != 0 && puddleDepth > 0.0f)
        {
            float blendWeight = puddleDepth * gMaterial.puddleColor.a;
            baseColor = lerp(baseColor, gMaterial.puddleColor.rgb, blendWeight);
            addedPuddleEmission = gMaterial.puddleColor.rgb * gMaterial.puddleEmission * puddleDepth;
        }

        // レイヤーのUVをずらしてサンプリングし、不規則な波紋アニメーションを作る
        float2 rippleUV = input.worldPosition.xz * gMaterial.rippleScale;
        float time = gFrameData.gTime * gMaterial.rippleSpeed;
        float3 combinedRipple = 0.0f.xxx;

        [unroll]
        for (int i = 0; i < 3; i++)
        {
            float2 offset = float2(i * 0.33f, i * 0.71f);
            float2 p = rippleUV + offset;
            float2 gridID = floor(p);
            float2 f = frac(p);

            // セルごとのランダムシード生成
            float3 seed = float3(gridID, float(i));
            float rand = frac(sin(dot(seed.xy + seed.z, float2(12.9898f, 78.233f))) * 43758.5453f);
            float localTime = frac(time * gMaterial.rippleFrequency + rand);

            float spread = localTime * gMaterial.rippleSize + 0.0001f;
            float2 animatedUV = (f - 0.5f) / spread + 0.5f;

            float3 r = 0.0f.xxx;
            if (animatedUV.x >= 0.0f && animatedUV.x <= 1.0f && animatedUV.y >= 0.0f && animatedUV.y <= 1.0f)
            {
                r = gRippleTexture.Sample(gSampler, animatedUV).xyz * 2.0f - 1.0f;
            }

            float mask = smoothstep(1.0f, 0.0f, localTime);
            float edgeMask = smoothstep(0.5f, 0.4f, length(f - 0.5f));
            combinedRipple += r * mask * edgeMask;
        }

        float3 rippleNormal = combinedRipple * gMaterial.rippleStrength * effectiveWetness;

        if (gMaterial.usePuddle != 0)
        {
            // 水たまり部分はベース法線を水平化してから波紋を適用
            float3 flatNormal = float3(0.0f, 1.0f, 0.0f);
            float3 baseN = normalize(lerp(normal, flatNormal, puddleDepth * 0.9f));
            normal = normalize(baseN + float3(rippleNormal.x, 0.0f, rippleNormal.y));
        }
        else
        {
            normal = normalize(normal + float3(rippleNormal.x, 0.0f, rippleNormal.y));
        }
    }
    
    // --------------------------------------------------------
    // Bubble (薄膜干渉)
    // --------------------------------------------------------
    float3 toEye = normalize(gFrameData.cameraWorldPosition - input.worldPosition);
    float bubbleAlpha = textureColor.a;
    
    if (gMaterial.isBubble != 0)
    {
        float3 bubbleNormal = normalize(input.normal);
        float NdotV = saturate(dot(bubbleNormal, toEye));
        float fresnel = 1.0f - NdotV;

        // コサインパレットによる色相シフト表現
        float t = fresnel + gFrameData.gTime * (gMaterial.wobbleSpeed * 0.1f);
        float3 a = 0.5f.xxx;
        float3 b = 0.5f.xxx;
        float3 c = 1.0f.xxx;
        float3 d = float3(0.00f, 0.33f, 0.67f);
        float3 rainbowColor = a + b * cos(6.28318f * (c * t + d));

        baseColor += rainbowColor * fresnel * gMaterial.rainbowIntensity;
        bubbleAlpha = lerp(0.1f, 1.0f, pow(fresnel, gMaterial.fresnelExponent));
    }
    
    // SurfaceData の構築
    SurfaceData surface;
    surface.albedo = baseColor * gMaterial.color.rgb;
    surface.pbrAlbedo = baseColor * pow(abs(gMaterial.color.rgb), 2.2f); // ガンマ補正
    surface.specularColor = gMaterial.specularColor.rgb;
    surface.normal = normal;
    surface.roughness = currentRoughness;
    surface.metalness = currentMetalness;
    surface.shininess = gMaterial.shininess;
    surface.diffuseReflection = gMaterial.diffuseReflection;
    surface.lightMode = gMaterial.lightMode;

    // --------------------------------------------------------
    // Lighting & IBL
    // --------------------------------------------------------
    float3 finalColor = 0.0f.xxx;
    
    if (gMaterial.enableLighting != 0)
    {
        finalColor += ApplyDirectionalLights(surface, toEyeWorld, shadowFactor, gDirectionalLights, gToonRamp, gClampSampler);
        finalColor += ApplyPointLights(surface, input.worldPosition, toEyeWorld, gPointLights);
        finalColor += ApplySpotLights(surface, input.worldPosition, toEyeWorld, gSpotLights);
        finalColor += ApplyAreaLights(surface, input.worldPosition, toEyeWorld, gAreaLights);

        if (gMaterial.lightMode == SHADING_MODEL_PBR)
        {
            float3 kS = F_SchlickRoughness(max(dot(surface.normal, toEyeWorld), 0.0f), 0.04f.xxx, surface.roughness);
            float3 kD = (1.0f.xxx - kS) * (1.0f - surface.metalness);
            
            float3 baseAmbient = 0.03f.xxx;
            float ambientOcclusion = lerp(gMaterial.shadowEnvStrength, 1.0f, shadowFactor);

            float3 ambientDiffuse = kD * surface.pbrAlbedo * baseAmbient;

            // IBL の計算。ラフネスに基づいてミップレベルを変える近似
            float3 reflectionVector = reflect(-toEyeWorld, surface.normal);
            float3 envColor = gEnvironmentTexture.SampleLevel(gSampler, reflectionVector, surface.roughness * 6.0f).rgb;
    
            float3 F0 = lerp(0.04f.xxx, surface.pbrAlbedo, surface.metalness);
            float3 F_env = F_SchlickRoughness(max(dot(surface.normal, toEyeWorld), 0.0f), F0, surface.roughness);
            float3 ambientSpecular = envColor * F_env;

            float3 ambient = (ambientDiffuse + ambientSpecular) * gMaterial.environmentMapIntensity;
            finalColor += ambient * ambientOcclusion;
        }
        else
        {
            // トゥーン等、非PBR時の環境マップフォールバック
            float3 reflectionVector = reflect(-toEyeWorld, surface.normal);
            float4 envColor = gEnvironmentTexture.Sample(gSampler, reflectionVector);
            float ambientOcclusion = lerp(gMaterial.shadowEnvStrength, 1.0f, shadowFactor);

            finalColor += envColor.rgb * gMaterial.environmentMapIntensity * ambientOcclusion;
        }
        
        if (gMaterial.enableRim != 0)
        {
            float3 toLight = float3(0.0f, 1.0f, 0.0f);
            if (gDirectionalLights[0].enable != 0)
                toLight = normalize(-gDirectionalLights[0].direction);
            
            finalColor += ApplyRimLight(
                surface.normal, toEyeWorld, toLight,
                gMaterial.rimPower, gMaterial.rimUseLightDir,
                gMaterial.rimColor, gMaterial.rimIntensity);
        }
    }
    else
    {
        // Unlit
        finalColor = surface.albedo;
    }
    
    finalColor *= input.worldColor.rgb;
    finalColor *= gMaterial.emissiveIntensity;
    finalColor += dissolveEdgeEmission;
    finalColor += addedPuddleEmission;

    // --------------------------------------------------------
    // G-Buffer Output
    // --------------------------------------------------------
    output.color.rgb = finalColor;
    output.color.a = (gMaterial.isBubble != 0) ? (bubbleAlpha * gMaterial.color.a) : (textureColor.a * gMaterial.color.a);
    output.normal = float4(normal, 1.0f);
    
    // R: Metalness, G: Roughness (遅延レンダリング用マテリアル情報)
    output.material = float4(currentMetalness, currentRoughness, 0.0f, 1.0f);
    
    // Velocity出力 (モーションブラー / TAA のReprojection用)
    // クリップ空間座標からNDCを求め、UV空間(Y反転)へ変換してフレーム間差分を計算
    float2 currentNDC = input.currentClipPos.xy / input.currentClipPos.w;
    float2 prevNDC = input.prevClipPos.xy / input.prevClipPos.w;

    float2 currentUV = currentNDC * float2(0.5f, -0.5f) + 0.5f;
    float2 prevUV = prevNDC * float2(0.5f, -0.5f) + 0.5f;

    output.velocity = currentUV - prevUV;
    
    return output;
}
