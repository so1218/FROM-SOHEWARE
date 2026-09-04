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
Texture2D<float4> gSceneColorTexture : register(t0);
Texture2D<float> gSceneDepthTexture : register(t1);
TextureCube<float4> gEnvironmentTexture : register(t2);
Texture2D<float4> gWaterNormalMap : register(t3);
Texture2D<float4> gRippleTexture : register(t4);

SamplerState gSampler : register(s0);
SamplerState gClampSampler : register(s2);

struct WaterPSOutput
{
    float4 color : SV_Target0; 
    float2 velocity : SV_Target1;
};

// 非線形デプスを線形距離(メートル)に変換
float LinearizeDepth(float ndcDepth)
{
    float nearP = gFrameData.nearClip;
    float farP = gFrameData.farClip;
    return (nearP * farP) / (farP - ndcDepth * (farP - nearP));
}

// 画面空間ノイズ（ジッタリング用）
float InterleavedGradientNoise(float2 pixelPos)
{
    float3 magic = float3(0.06711056f, 0.00583715f, 52.9829189f);
    return frac(magic.z * frac(dot(pixelPos, magic.xy)));
}

// RNM (Reoriented Normal Mapping) による高度な法線合成
float3 BlendNormalsRNM(float3 baseNormal, float3 detailNormal)
{
    float3 t = baseNormal + float3(0.0f, 0.0f, 1.0f);
    float3 u = detailNormal * float3(-1.0f, -1.0f, 1.0f);
    return t * dot(t, u) / t.z - u;
}

// 高精度SSR (ジッタリング + 二分探索精度の向上)
float4 RaycastWaterSSR(float3 rayOriginView, float3 rayDirView, float2 pixelCoord)
{
    static const int MAX_STEPS = 24;
    static const int BINARY_SEARCH_STEPS = 5;
    
    float jitter = InterleavedGradientNoise(pixelCoord);
    float stepSize = 0.8f;
    float3 currentPos = rayOriginView + rayDirView * (stepSize * jitter);
    float3 stepVec = rayDirView * stepSize;

    [unroll(MAX_STEPS)]
    for (int i = 0; i < MAX_STEPS; ++i)
    {
        float4 clipPos = mul(float4(currentPos, 1.0f), gFrameData.projectionMatrix);
        float2 uv = (clipPos.xy / clipPos.w) * float2(0.5f, -0.5f) + 0.5f;

        if (any(uv < 0.0f) || any(uv > 1.0f))
            break;

        float sceneLinearZ = LinearizeDepth(gSceneDepthTexture.SampleLevel(gSampler, uv, 0).r);
        float depthDiff = currentPos.z - sceneLinearZ;

        // レイがシーンに交差した場合
        if (depthDiff > 0.0f && depthDiff < 2.0f)
        {
            // 二分探索 (Binary Search) で交差点の精度を極限まで高める
            float3 minPos = currentPos - stepVec;
            float3 maxPos = currentPos;
            float3 midPos = currentPos;

            for (int j = 0; j < BINARY_SEARCH_STEPS; ++j)
            {
                midPos = lerp(minPos, maxPos, 0.5f);
                float4 midClip = mul(float4(midPos, 1.0f), gFrameData.projectionMatrix);
                float2 midUV = (midClip.xy / midClip.w) * float2(0.5f, -0.5f) + 0.5f;
                float midSceneZ = LinearizeDepth(gSceneDepthTexture.SampleLevel(gSampler, midUV, 0).r);

                if (midPos.z > midSceneZ)
                    maxPos = midPos;
                else
                    minPos = midPos;
            }

            float4 finalClip = mul(float4(midPos, 1.0f), gFrameData.projectionMatrix);
            float2 finalUV = (finalClip.xy / finalClip.w) * float2(0.5f, -0.5f) + 0.5f;

            // 画面端のフェードアウト
            float2 edgeFade = smoothstep(0.0f, 0.15f, min(finalUV, 1.0f - finalUV));
            float alpha = edgeFade.x * edgeFade.y;

            float3 color = gSceneColorTexture.SampleLevel(gSampler, finalUV, 0).rgb;
            return float4(color, alpha);
        }
        currentPos += stepVec;
    }
    return float4(0.0f, 0.0f, 0.0f, 0.0f);
}

// プロシージャル・水底コースティクス (ゆらぎ光) の計算
float3 CalculateCaustics(float2 worldXZ, float3 lightDir, float time, float waterDepth)
{
    float2 uv1 = worldXZ * gWaterMaterial.causticsScale + lightDir.xz * (time * 0.05f);
    float2 uv2 = worldXZ * (gWaterMaterial.causticsScale * 1.3f) - lightDir.xz * (time * 0.07f);

    float3 n1 = gWaterNormalMap.Sample(gSampler, uv1).rgb * 2.0f - 1.0f;
    float3 n2 = gWaterNormalMap.Sample(gSampler, uv2).rgb * 2.0f - 1.0f;

    float3 combinedN = normalize(n1 + n2);
    float causticsPattern = pow(saturate(dot(combinedN, float3(0.0f, 1.0f, 0.0f))), 12.0f);

    // 水深による減衰（浅瀬で最も強く、深い場所で消える）
    float depthFade = exp(-waterDepth * 0.3f) * smoothstep(0.1f, 0.5f, waterDepth);
    return causticsPattern * gWaterMaterial.causticsIntensity * depthFade * gDirectionalLights[0].color.rgb;
}

WaterPSOutput main(PixelShaderInput input)
{
    PixelShaderOutput output;

    float3 toEyeWorld = normalize(gFrameData.cameraWorldPosition - input.worldPosition);
    float2 screenUV = (input.currentClipPos.xy / input.currentClipPos.w) * float2(0.5f, -0.5f) + 0.5f;

    float sceneRawDepth = gSceneDepthTexture.Sample(gSampler, screenUV).r;
    float sceneLinearDepth = LinearizeDepth(sceneRawDepth);
    float waterLinearDepth = LinearizeDepth(input.currentClipPos.z / input.currentClipPos.w);
    float waterDepth = max(sceneLinearDepth - waterLinearDepth, 0.0f);

    // --------------------------------------------------------
    // RNM法線多重スクロール（大波 + 小波）
    // --------------------------------------------------------
    float time = gFrameData.gTime * gWaterMaterial.waveSpeed;
    float2 uv1 = input.worldPosition.xz * gWaterMaterial.waveTiling.x + float2(0.01f, 0.015f) * time;
    float2 uv2 = input.worldPosition.xz * gWaterMaterial.waveTiling.y + float2(-0.02f, 0.01f) * time;

    float3 n1 = gWaterNormalMap.Sample(gSampler, uv1).rgb * 2.0f - 1.0f;
    float3 n2 = gWaterNormalMap.Sample(gSampler, uv2).rgb * 2.0f - 1.0f;

    // 単純加算ではなくRNMで精密に法線をブレンド
    float3 baseWaveTS = BlendNormalsRNM(n1, n2);

    // 雨の波紋をさらにRNMで重ね合わせ
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
        combinedRippleTS *= gWaterMaterial.rippleStrength * gWaterMaterial.rainIntensity;
    }

    float3 finalTangentNormal = normalize(BlendNormalsRNM(baseWaveTS, combinedRippleTS));

    // ワールド法線変換 (TBN)
    float3 N = normalize(input.normal);
    float3 T = normalize(input.tangent);
    float3 B = normalize(cross(N, T));
    float3x3 TBN = float3x3(T, B, N);
    float3 worldNormal = normalize(mul(finalTangentNormal, TBN));

    // --------------------------------------------------------
    // 屈折 & 動的コースティクス (Caustics)
    // --------------------------------------------------------
    float distortion = smoothstep(0.0f, 0.5f, waterDepth) * gWaterMaterial.refractionAmount;
    float2 refractUV = screenUV + finalTangentNormal.xy * distortion;

    if (LinearizeDepth(gSceneDepthTexture.Sample(gSampler, refractUV).r) < waterLinearDepth)
    {
        refractUV = screenUV;
    }
    float3 sceneColor = gSceneColorTexture.Sample(gSampler, refractUV).rgb;

    // 水底コースティクス計算・合成
    float3 lightDir = normalize(-gDirectionalLights[0].direction);
    float3 caustics = CalculateCaustics(input.worldPosition.xz, lightDir, gFrameData.gTime, waterDepth);
    sceneColor += caustics;

    // Beer-Lambert則
    float transmittance = exp(-waterDepth * gWaterMaterial.absorption);
    float3 waterBodyColor = lerp(gWaterMaterial.deepColor.rgb, gWaterMaterial.shallowColor.rgb, transmittance);
    float3 refractedLight = sceneColor * waterBodyColor;

    // --------------------------------------------------------
    // PBR Direct Lighting
    // --------------------------------------------------------
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

    float3 directSpecular = 0.0f.xxx;
    directSpecular += ApplyDirectionalLights(surface, toEyeWorld, 1.0f, gDirectionalLights, gSceneColorTexture, gClampSampler);
    directSpecular += ApplyPointLights(surface, input.worldPosition, toEyeWorld, gPointLights);
    directSpecular += ApplySpotLights(surface, input.worldPosition, toEyeWorld, gSpotLights);

    // --------------------------------------------------------
    // 高精度SSR & 反射合成
    // --------------------------------------------------------
    float NdotV = saturate(dot(worldNormal, toEyeWorld));
    float fresnel = 0.02f + (1.0f - 0.02f) * pow(1.0f - NdotV, 5.0f);

    float3 reflectVector = reflect(-toEyeWorld, worldNormal);
    float3 skyReflection = gEnvironmentTexture.SampleLevel(gSampler, reflectVector, surface.roughness * 6.0f).rgb;

    float3 viewPos = mul(float4(input.worldPosition, 1.0f), gFrameData.viewMatrix).xyz;
    float3 viewNormal = normalize(mul(worldNormal, (float3x3) gFrameData.viewMatrix));
    float3 viewDir = normalize(viewPos);
    float3 reflectDirView = reflect(viewDir, viewNormal);

    // 高精度SSRの実行 (ジッタリング位置用としてピクセル座標を渡す)
    float4 ssrResult = RaycastWaterSSR(viewPos, reflectDirView, input.currentClipPos.xy);
    float3 combinedReflection = lerp(skyReflection, ssrResult.rgb, ssrResult.a);

    float3 finalColor = lerp(refractedLight, combinedReflection, fresnel) + directSpecular;
    float edgeAlpha = smoothstep(0.0f, 0.15f, waterDepth) * gWaterMaterial.shallowColor.a;

    // 出力
    output.color = float4(finalColor, edgeAlpha);
    
    float2 currentNDC = input.currentClipPos.xy / input.currentClipPos.w;
    float2 prevNDC = input.prevClipPos.xy / input.prevClipPos.w;
    output.velocity = (currentNDC * float2(0.5f, -0.5f) + 0.5f) - (prevNDC * float2(0.5f, -0.5f) + 0.5f);

    return output;
}