#include "Object3D.hlsli"
#include "ShaderConstants.hlsli"
#include "LightingUtils.hlsli"
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

// 専用マテリアルバッファ
ConstantBuffer<WaterMaterialData> gWaterMaterial : register(b5);

// Textures & Samplers
Texture2D<float4> gSceneColorTexture : register(t0); // バックバッファコピー
Texture2D<float> gSceneDepthTexture : register(t1); // 深度バッファコピー
TextureCube<float4> gEnvironmentTexture : register(t2); // IBL/スカイ反射
Texture2D<float4> gWaterNormalMap : register(t3); // ベース波の法線
Texture2D<float4> gRippleTexture : register(t4); // 雨の波紋用ノーマルマップ

SamplerState gSampler : register(s0);
SamplerState gClampSampler : register(s2);

// 非線形デプスをカメラからの線形距離(メートル)に変換
float LinearizeDepth(float ndcDepth)
{
    float nearP = gFrameData.nearClip;
    float farP = gFrameData.farClip;
    return (nearP * farP) / (farP - ndcDepth * (farP - nearP));
}

float4 RaycastWaterSSR(float3 rayOriginView, float3 rayDirView)
{
    // ステップ幅を広げ、距離に応じて可変にするかステップ数を調整
    float3 stepVec = rayDirView * 1.0f;
    float3 currentPos = rayOriginView + stepVec;

    for (int i = 0; i < 16; ++i)
    {
        float4 clipPos = mul(float4(currentPos, 1.0f), gFrameData.projectionMatrix);
        float2 uv = clipPos.xy / clipPos.w * float2(0.5f, -0.5f) + 0.5f;

        if (any(uv < 0.0f) || any(uv > 1.0f))
            break;

        float sceneLinearZ = LinearizeDepth(gSceneDepthTexture.SampleLevel(gSampler, uv, 0).r);

        // 遮蔽判定
        if (currentPos.z > sceneLinearZ && (currentPos.z - sceneLinearZ) < 1.5f)
        {
            // 画面端に行くほどアルファを落としてカットオフを防ぐ
            float2 edgeFade = min(uv, 1.0f - uv) * 10.0f;
            float alpha = saturate(edgeFade.x) * saturate(edgeFade.y);

            float3 color = gSceneColorTexture.SampleLevel(gSampler, uv, 0).rgb;
            return float4(color, alpha); // 色とヒットアルファを一緒に返す
        }
        currentPos += stepVec;
    }
    return float4(0.0f, 0.0f, 0.0f, 0.0f); // ヒット失敗 (Alpha = 0)
}

PixelShaderOutput main(PixelShaderInput input)
{
    PixelShaderOutput output;

    float3 toEyeWorld = normalize(gFrameData.cameraWorldPosition - input.worldPosition);

    // --------------------------------------------------------
    // 1. スクリーンUV & 水深(Depth Fade)計算
    // --------------------------------------------------------
    float2 screenUV = (input.currentClipPos.xy / input.currentClipPos.w) * float2(0.5f, -0.5f) + 0.5f;

    float sceneRawDepth = gSceneDepthTexture.Sample(gSampler, screenUV).r;
    float sceneLinearDepth = LinearizeDepth(sceneRawDepth);
    float waterLinearDepth = LinearizeDepth(input.currentClipPos.z / input.currentClipPos.w);

    float waterDepth = max(sceneLinearDepth - waterLinearDepth, 0.0f);

    // --------------------------------------------------------
    // 2. 風による基本の波（2重法線スクロール）
    // --------------------------------------------------------
    float time = gFrameData.gTime * gWaterMaterial.waveSpeed;
    float2 uv1 = input.worldPosition.xz * gWaterMaterial.waveTiling.x + float2(0.01f, 0.015f) * time;
    float2 uv2 = input.worldPosition.xz * gWaterMaterial.waveTiling.y + float2(-0.02f, 0.01f) * time;

    float3 n1 = gWaterNormalMap.Sample(gSampler, uv1).rgb * 2.0f - 1.0f;
    float3 n2 = gWaterNormalMap.Sample(gSampler, uv2).rgb * 2.0f - 1.0f;
    float3 baseWaveTS = normalize(n1 + n2);

    // --------------------------------------------------------
    // 3. 雨の降雨波紋 (Rain Ripple System)
    // --------------------------------------------------------
    float3 combinedRippleTS = 0.0f.xxx;
    
    if (gWaterMaterial.rainIntensity > 0.0f)
    {
        float2 rippleUV = input.worldPosition.xz * gWaterMaterial.rippleScale;
        float rTime = gFrameData.gTime * gWaterMaterial.rippleSpeed;

        [unroll]
        for (int i = 0; i < 3; i++)
        {
            float2 offset = float2(i * 0.33f, i * 0.71f);
            float2 p = rippleUV + offset;
            float2 gridID = floor(p);
            float2 f = frac(p);

            float3 seed = float3(gridID, float(i));
            float rand = frac(sin(dot(seed.xy + seed.z, float2(12.9898f, 78.233f))) * 43758.5453f);
            float localTime = frac(rTime * 1.2f + rand);

            float spread = localTime * 0.8f + 0.0001f;
            float2 animatedUV = (f - 0.5f) / spread + 0.5f;

            float3 r = 0.0f.xxx;
            if (animatedUV.x >= 0.0f && animatedUV.x <= 1.0f && animatedUV.y >= 0.0f && animatedUV.y <= 1.0f)
            {
                r = gRippleTexture.Sample(gSampler, animatedUV).xyz * 2.0f - 1.0f;
            }

            float mask = smoothstep(1.0f, 0.0f, localTime);
            float edgeMask = smoothstep(0.5f, 0.4f, length(f - 0.5f));
            combinedRippleTS += r * mask * edgeMask;
        }
    }

    // 基本波と雨波紋の合成
    float3 finalTangentNormal = normalize(baseWaveTS + combinedRippleTS * gWaterMaterial.rippleStrength * gWaterMaterial.rainIntensity);

    // ワールド法線へ変換 (TBN)
    float3 N = normalize(input.normal);
    float3 T = normalize(input.tangent);
    float3 B = normalize(cross(N, T));
    float3x3 TBN = float3x3(T, B, N);
    float3 worldNormal = normalize(mul(finalTangentNormal, TBN));

    // --------------------------------------------------------
    // 4. 画面空間屈折 (Refraction)
    // --------------------------------------------------------
    float distortion = smoothstep(0.0f, 0.5f, waterDepth) * gWaterMaterial.refractionAmount;
    float2 refractUV = screenUV + finalTangentNormal.xy * distortion;

    // 手前のオブジェクトを誤って引っ張らない保護
    float refRawDepth = gSceneDepthTexture.Sample(gSampler, refractUV).r;
    if (LinearizeDepth(refRawDepth) < waterLinearDepth)
    {
        refractUV = screenUV;
    }
    float3 sceneColor = gSceneColorTexture.Sample(gSampler, refractUV).rgb;

    // --------------------------------------------------------
    // 5. Beer-Lambert則による水質・吸光表現
    // --------------------------------------------------------
    float transmittance = exp(-waterDepth * gWaterMaterial.absorption);
    float3 waterBodyColor = lerp(gWaterMaterial.deepColor.rgb, gWaterMaterial.shallowColor.rgb, transmittance);
    float3 refractedLight = sceneColor * waterBodyColor;

    // --------------------------------------------------------
    // 6. 各種光源によるダイナミックライティング (Lighting)
    // --------------------------------------------------------
    // 水面用の仮定 SurfaceData を構築 (鏡面ハイライトを強調するためAlbedoは黒扱い)
    SurfaceData surface;
    surface.albedo = 0.0f.xxx;
    surface.pbrAlbedo = 0.0f.xxx;
    surface.specularColor = 1.0f.xxx * gWaterMaterial.specularIntensity;
    surface.normal = worldNormal;
    surface.roughness = clamp(gWaterMaterial.roughness, 0.01f, 1.0f);
    surface.metalness = 0.0f;
    surface.shininess = 128.0f;
    surface.diffuseReflection = 1.0f;
    surface.lightMode = SHADING_MODEL_PBR;

    // Direct Lights の鏡面反射（ハイライト）を加算
    float3 directSpecular = 0.0f.xxx;
    directSpecular += ApplyDirectionalLights(surface, toEyeWorld, 1.0f, gDirectionalLights, gSceneColorTexture, gClampSampler);
    directSpecular += ApplyPointLights(surface, input.worldPosition, toEyeWorld, gPointLights);
    directSpecular += ApplySpotLights(surface, input.worldPosition, toEyeWorld, gSpotLights);

    // --------------------------------------------------------
    // 7. フレネル & 反射 (SSR + IBL Reflection)
    // --------------------------------------------------------
   // --------------------------------------------------------
    // 7. フレネル & 反射 (Water-Space SSR + IBL Reflection)
    // --------------------------------------------------------
    float NdotV = saturate(dot(worldNormal, toEyeWorld));
    float fresnel = 0.02f + (1.0f - 0.02f) * pow(1.0f - NdotV, 5.0f);

    // スカイ反射（フォールバック）
    float3 reflectVector = reflect(-toEyeWorld, worldNormal);
    float3 skyReflection = gEnvironmentTexture.SampleLevel(gSampler, reflectVector, surface.roughness * 6.0f).rgb;

    // View空間での水面レイマーチ準備
    float3 viewPos = mul(float4(input.worldPosition, 1.0f), gFrameData.viewMatrix).xyz;
    float3 viewNormal = normalize(mul(worldNormal, (float3x3) gFrameData.viewMatrix));
    float3 viewDir = normalize(viewPos);
    float3 reflectDirView = reflect(viewDir, viewNormal);

    // 水面からのリアルタイムSSRの実行
    float3 ssrColor = RaycastWaterSSR(viewPos, reflectDirView, worldNormal);
    
    // レイがヒットした場合はSSR、画面外などはスカイ反射へフォールバック
    float ssrMask = saturate(length(ssrColor));
    float3 combinedReflection = lerp(skyReflection, ssrColor, ssrMask);

    // 最終カラー合成 (1回のみフレネルを適用)
    float3 finalColor = lerp(refractedLight, combinedReflection, fresnel) + directSpecular;

    // 岸辺 (waterDepth == 0) のアルファ溶け込み
    float edgeAlpha = smoothstep(0.0f, 0.15f, waterDepth) * gWaterMaterial.shallowColor.a;

    // --------------------------------------------------------
    // 8. G-Buffer Output
    // --------------------------------------------------------
    output.color = float4(finalColor, edgeAlpha);
    output.normal = float4(worldNormal, 1.0f);
    output.material = float4(0.0f, surface.roughness, 0.0f, 1.0f);

    // Velocity 出力
    float2 currentNDC = input.currentClipPos.xy / input.currentClipPos.w;
    float2 prevNDC = input.prevClipPos.xy / input.prevClipPos.w;
    float2 currentUV = currentNDC * float2(0.5f, -0.5f) + 0.5f;
    float2 prevUV = prevNDC * float2(0.5f, -0.5f) + 0.5f;
    output.velocity = currentUV - prevUV;

    return output;
}