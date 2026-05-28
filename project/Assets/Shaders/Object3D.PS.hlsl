#include "Object3D.hlsli"
#include "ShaderConstants.hlsli"

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

Texture2D<float4> gTexture : register(t0);
TextureCube<float4> gEnvironmentTexture : register(t1);
Texture2D<float> gShadowMap : register(t2);
Texture2D<float4> gToonRamp : register(t3);
Texture2D<float4> gDissolveTexture : register(t4);
Texture2D<float3> gNormalTexture : register(t5);
Texture2D<float3> gRippleTexture : register(t6);
Texture2D<float> gPuddleNoiseTexture : register(t7);

SamplerState gSampler : register(s0);
SamplerComparisonState gShadowSampler : register(s1);
SamplerState gClampSampler : register(s2);

float DitherThreshold4x4(int2 position);

float3 DrawArtGridColor(PixelShaderInput input);
bool ShouldDiscardArtGrid(PixelShaderInput input);

float3 ApplyDirectionalLights(float3 baseColor, float3 pbrAlbedo, float3 normal, float3 toEye, float shadowFactor);
float3 ApplyPointLights(float3 baseColor, float3 pbrAlbedo, float3 normal, float3 worldPos, float3 toEye);
float3 ApplySpotLights(float3 baseColor, float3 pbrAlbedo, float3 normal, float3 worldPos, float3 toEye);
float3 ApplyAreaLights(float3 baseColor, float3 pbrAlbedo, float3 normal, float3 worldPos, float3 toEye);
float3 ApplyRimLight(float3 normal, float3 toEye, float3 toLight);

float3 F_Schlick(float cosTheta, float3 F0);
float D_GGX(float3 N, float3 H, float roughness);
float G_SchlickGGX(float NdotV, float roughness);
float G_Smith(float3 N, float3 V, float3 L, float roughness);
float3 CalculatePBR(
    float3 albedo,
    float3 N,
    float3 V,
    float3 L,
    float3 lightColor,
    float lightIntensity,
    float roughness,
    float metalness
);

// 影の濃さを計算する関数
float CalculateShadow(float4 shadowCoord, float3 normal);
float3 CalculateNormalFromMap(PixelShaderInput input, float3 normal, float2 uv);
float3 CalculateTriplanarNormal(float3 worldPos, float3 worldNormal, float texScale, float blendSharpness);
float4 CalculateTriplanarColor(float3 worldPos, float3 worldNormal, float texScale, float blendSharpness);

PixelShaderOutput main(PixelShaderInput input)
{
    PixelShaderOutput output;

    // ベースとなるワールド法線を計算
    float3 worldNormal = normalize(input.normal);

    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float4 textureColor;
    
    // トライプラナーのブレンド度合い
    float blendSharpness = gMaterial.triplanarBlendSharpness > 0.0f ? gMaterial.triplanarBlendSharpness : 4.0f;

    // トライプラナー有効/無効でカラー取得を分岐
    if (gMaterial.useTriplanar != 0)
    {
        textureColor = CalculateTriplanarColor(input.worldPosition, worldNormal, gMaterial.triplanarScale, blendSharpness);
    }
    else
    {
        textureColor = gTexture.Sample(gSampler, transformedUV.xy);
    }
    
    float3 baseColor = textureColor.rgb;

    // ディゾルブ処理
    float3 dissolveEdgeEmission = float3(0, 0, 0);

    // マテリアル設定でディゾルブが有効、かつ閾値が0より大きい場合のみ計算
    if (gMaterial.enableDissolve != 0)
    {
        // ノイズテクスチャをサンプリング
        float noiseValue = gDissolveTexture.Sample(gSampler, transformedUV.xy).r;

        // ノイズの値が閾値より低ければピクセルを捨てる
        if (noiseValue <= gMaterial.dissolveThreshold)
        {
            discard;
        }

        // 境界線の発光
        float difference = noiseValue - gMaterial.dissolveThreshold;
        
        // エッジ幅の範囲内なら発光させる
        if (difference < gMaterial.edgeWidth)
        {
            // differenceが小さいほど1.0に近づくように反転
            float t = 1.0f - (difference / gMaterial.edgeWidth);

            // グラデーションを滑らかに
            t = smoothstep(0.0f, 1.0f, t);

            // 高輝度カラーの計算
            dissolveEdgeEmission = gMaterial.edgeColor * t * gMaterial.edgeIntensity;
        }
    }

    // グリッド適用
    if (gMaterial.isArtGrid)
    {
        if (ShouldDiscardArtGrid(input))
        {
            discard;
        }

        output.color.rgb = DrawArtGridColor(input);
        output.color.a = 1.0;
        // グリッドの法線は上、材質は適当な値
        output.normal = float4(0.0f, 1.0f, 0.0f, 1.0f);
        output.material = float4(0.0f, 1.0f, 0.0f, 1.0f);
        
        return output;
    }
    
    // 影の計算 
    float shadowFactor = 1.0f;
    
    // 0番目のライトが有効なら影を計算
    if (gDirectionalLights[0].enable && gMaterial.addShadow != 0)
    {
        shadowFactor = CalculateShadow(input.shadowCoord, normalize(input.normal));
    }
    
    // ライティング処理
    float3 finalColor = float3(0.0f, 0.0f, 0.0f);
    float3 normal;
    
    if (gMaterial.enableNormalMap != 0)
    {
        if (gMaterial.useTriplanar != 0)
        {
            // トライプラナーで法線を計算
            normal = CalculateTriplanarNormal(input.worldPosition, worldNormal, gMaterial.triplanarScale, blendSharpness);
            
            // 法線の強さを適用してブレンド
            normal = normalize(lerp(worldNormal, normal, gMaterial.normalIntensity));
        }
        else
        {
            // 従来のUVマッピングでの法線計算
            normal = CalculateNormalFromMap(input, worldNormal, transformedUV.xy);
        }
    }
    else
    {
        normal = worldNormal;
    }
    
    // 現在のラフネスとメタルネスを変数化
    float currentRoughness = clamp(gMaterial.roughness, 0.05f, 1.0f);
    float currentMetalness = saturate(gMaterial.metalness);
    
    // 水たまりの発光
    float3 addedPuddleEmission = float3(0, 0, 0);
    
    // 波紋と水たまりの処理
    if (gMaterial.enableRipple != 0 && gMaterial.wetness > 0.0f)
    {
        // 基本的な全体の濡れ具合
        float globalWetness = gMaterial.wetness;
        float puddleDepth = 0.0f; // 水たまりの深さ

        // 水たまりマスクの計算
        if (gMaterial.usePuddle != 0)
        {
            // ノイズに基づいて水が溜まる場所を特定
            float2 puddleUV = input.worldPosition.xz * gMaterial.puddleScale;
            float noiseVal = gPuddleNoiseTexture.Sample(gSampler, puddleUV).r;
            float edgeSoftness = max(gMaterial.puddleFalloff, 0.001f);
            
            // 水たまりの深さ
            puddleDepth = smoothstep(gMaterial.wetness, gMaterial.wetness - edgeSoftness, noiseVal);
        }

        // 最終的な濡れ度
        float effectiveWetness = max(globalWetness, puddleDepth);

        // 濡れている部分の質感
        currentRoughness = lerp(currentRoughness, 0.01f, effectiveWetness);

        // 水たまりの色・透明度・発光の適用
        if (gMaterial.usePuddle != 0 && puddleDepth > 0.0f)
        {
            // 最終的なブレンド率を決定
            float blendWeight = puddleDepth * gMaterial.puddleColor.a;
            
            // 下地に水たまりの色をブレンド
            baseColor = lerp(baseColor, gMaterial.puddleColor.rgb, blendWeight);
            
            // 発光成分の計算
            addedPuddleEmission = gMaterial.puddleColor.rgb * gMaterial.puddleEmission * puddleDepth;
        }

        // 波紋の計算
        float2 rippleUV = input.worldPosition.xz * gMaterial.rippleScale;
        float time = gFrameData.gTime * gMaterial.rippleSpeed;
        float3 combinedRipple = float3(0, 0, 0);

        for (int i = 0; i < 3; i++)
        {
            float2 offset = float2(i * 0.33, i * 0.71);
            float2 p = rippleUV + offset;
            float2 gridID = floor(p);
            float2 f = frac(p);

            float3 seed = float3(gridID, float(i));
            float rand = frac(sin(dot(seed.xy + seed.z, float2(12.9898, 78.233))) * 43758.5453);
            
            float localTime = frac(time * gMaterial.rippleFrequency + rand);

            float spread = localTime * gMaterial.rippleSize + 0.0001;
            float2 animatedUV = (f - 0.5) / spread + 0.5;

            float3 r = float3(0, 0, 0);
            if (animatedUV.x >= 0.0 && animatedUV.x <= 1.0 && animatedUV.y >= 0.0 && animatedUV.y <= 1.0)
            {
                r = gRippleTexture.Sample(gSampler, animatedUV).xyz * 2.0f - 1.0f;
            }

            float mask = smoothstep(1.0, 0.0, localTime);
            float edgeMask = smoothstep(0.5, 0.4, length(f - 0.5));

            combinedRipple += r * mask * edgeMask;
        }

        // 波紋の強さは全体の濡れ具合に合わせて掛ける
        float3 rippleNormal = combinedRipple * gMaterial.rippleStrength * effectiveWetness;

        // 法線の合成
        if (gMaterial.usePuddle != 0)
        {
            // 水が溜まっている部分だけ地面を平坦に
            float3 flatNormal = float3(0, 1, 0);
            // puddleDepthが高いほど平らに
            float3 baseN = normalize(lerp(normal, flatNormal, puddleDepth * 0.9f));
            
            // 平らにした地面の上に、全体に降っている波紋を乗せる
            normal = normalize(baseN + float3(rippleNormal.x, 0.0f, rippleNormal.y));
        }
        else
        {
            // 元の法線を維持して波紋だけ乗せる
            normal = normalize(normal + float3(rippleNormal.x, 0.0f, rippleNormal.y));
        }
    }
    
    float3 toEye = normalize(gFrameData.cameraWorldPosition - input.worldPosition);
    
    // バブル処理
    float bubbleAlpha = textureColor.a;
    if (gMaterial.isBubble != 0)
    {
        float3 bubbleNormal = normalize(input.normal);
        float NdotV = saturate(dot(bubbleNormal, toEye));
        float fresnel = 1.0f - NdotV;

        // 虹色の計算
        float t = fresnel + gFrameData.gTime * (gMaterial.wobbleSpeed * 0.1f);
        float3 a = float3(0.5, 0.5, 0.5);
        float3 b = float3(0.5, 0.5, 0.5);
        float3 c = float3(1.0, 1.0, 1.0);
        float3 d = float3(0.00, 0.33, 0.67);
        float3 rainbowColor = a + b * cos(6.28318 * (c * t + d));

        // ベースカラーに虹色を乗せる
        baseColor += rainbowColor * fresnel * gMaterial.rainbowIntensity;
        
        // 縁を不透明にする
        bubbleAlpha = lerp(0.1f, 1.0f, pow(fresnel, gMaterial.fresnelExponent));
    }

    if (gMaterial.enableLighting != 0)
    {
        float3 pbrAlbedo = baseColor * pow(gMaterial.color.rgb, 2.2f);
        
        // Directional Light
        finalColor += ApplyDirectionalLights(baseColor, pbrAlbedo, normal, toEye, shadowFactor);

        // Point Light
        finalColor += ApplyPointLights(baseColor, pbrAlbedo, normal, input.worldPosition, toEye);

        // Spot Light
        finalColor += ApplySpotLights(baseColor, pbrAlbedo, normal, input.worldPosition, toEye);
        
        // Area Light
        finalColor += ApplyAreaLights(baseColor, pbrAlbedo, normal, input.worldPosition, toEye);
        
        // 雷フラッシュの共通準備
        float flashIntensity = gFrameData.lightningFlashIntensity;
        float3 flashColor = gFrameData.lightningFlashColor * flashIntensity;
        float flashShadowCancel = saturate(flashIntensity);

        // 環境マップ処理
        if (gMaterial.lightMode == SHADING_MODEL_PBR)
        {
            // 拡散反射
            float3 kS = F_Schlick(max(dot(normal, toEye), 0.0f), float3(0.04f, 0.04f, 0.04f));
            float3 kD = 1.0f - kS;
            kD *= (1.0f - currentMetalness);
            
            float3 baseAmbient = float3(0.03f, 0.03f, 0.03f);
            
            // 影の計算（フラッシュ時は影を打ち消す）
            float ambientOcclusion = lerp(gMaterial.shadowEnvStrength, 1.0f, shadowFactor);
            ambientOcclusion = lerp(ambientOcclusion, 1.0f, flashShadowCancel);

            // アンビエントディフューズにフラッシュを加算
            float3 ambientDiffuse = kD * pbrAlbedo * (baseAmbient + flashColor);

            // 鏡面反射
            float3 reflectionVector = reflect(-toEye, normal);
            float3 envColor = gEnvironmentTexture.SampleLevel(gSampler, reflectionVector, currentRoughness * 6.0f).rgb;
            
            // 空の反射（環境マップ）自体をフラッシュで発光
            envColor += flashColor;
    
            float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), pbrAlbedo, currentMetalness);
            float3 F_env = F_Schlick(max(dot(normal, toEye), 0.0f), F0);
            float3 ambientSpecular = envColor * F_env;

            // 拡散反射と鏡面反射の合成
            float3 ambient = (ambientDiffuse + ambientSpecular) * gMaterial.environmentMapIntensity;
     
            finalColor += ambient * ambientOcclusion;
        }
        else
        {
            // 単純な環境マッピング
            float3 reflectionVector = reflect(-toEye, normal);
            float4 envColor = gEnvironmentTexture.Sample(gSampler, reflectionVector);

            // 影の計算（フラッシュ時は影を打ち消す）
            float ambientOcclusion = lerp(gMaterial.shadowEnvStrength, 1.0f, shadowFactor);
            ambientOcclusion = lerp(ambientOcclusion, 1.0f, flashShadowCancel);

            // 環境光の加算
            finalColor += envColor.rgb * gMaterial.environmentMapIntensity * ambientOcclusion;

            // ベースカラーに対して、フラッシュの色と強さをそのまま乗せる
            finalColor += baseColor * gMaterial.color.rgb * flashColor;
        }
        
        if (gMaterial.enableRim != 0)
        {
            // メインライトの方向を取得
            float3 toLight = float3(0, 1, 0);
            if (gDirectionalLights[0].enable != 0)
            {
                // ライトの向きの逆ベクトル（光源へのベクトル）
                toLight = normalize(-gDirectionalLights[0].direction);
            }

            // ライト方向を渡す
            finalColor += ApplyRimLight(normal, toEye, toLight);
        }
       
    }
    else
    {
        finalColor = baseColor * gMaterial.color.rgb;
    }
    
    finalColor *= input.worldColor.rgb;
    
     // 自己発光を加算
    finalColor *= gMaterial.emissiveIntensity;
    
    // ディゾルブのエッジ発光を加算
    finalColor += dissolveEdgeEmission;
    
    // 水たまりの発光を加算
    finalColor += addedPuddleEmission;

    output.color.rgb = finalColor;

    if (gMaterial.isBubble != 0)
    {
        output.color.a = bubbleAlpha * gMaterial.color.a;
    }
    else
    {
        output.color.a = textureColor.a * gMaterial.color.a;
    }
  
    // ディザー透明処理
    if (output.color.a <= gMaterial.alphaTestThreshold)
    {
        discard;
    }
    
    // G-Bufferへの情報書き込み

    // 法線情報
    output.normal = float4(normal, 1.0f);

    // 材質情報
    // R=メタルネス, G=ラフネス
    output.material = float4(currentMetalness, currentRoughness, 0.0f, 1.0f);
    
    return output;
}

float DitherThreshold4x4(int2 position)
{
    int2 pos = position % 4;

    float bayer4x4[16] =
    {
        0.0 / 16.0, 8.0 / 16.0, 2.0 / 16.0, 10.0 / 16.0,
        12.0 / 16.0, 4.0 / 16.0, 14.0 / 16.0, 6.0 / 16.0,
        3.0 / 16.0, 11.0 / 16.0, 1.0 / 16.0, 9.0 / 16.0,
        15.0 / 16.0, 7.0 / 16.0, 13.0 / 16.0, 5.0 / 16.0,
    };

    return bayer4x4[pos.y * 4 + pos.x];
}

float gridLine(float2 uv, float scale, float thickness)
{
    float2 grid = abs(frac(uv * scale - 0.5) - 0.5) / fwidth(uv * scale);
    float line1 = min(grid.x, grid.y);
    return smoothstep(0.0, thickness, line1);
}

float3 DrawArtGridColor(PixelShaderInput input)
{
    // 画面解像度を考慮したフラグメント座標
    float2 fragCoord = input.texcoord * gFrameData.iResolution;

    // UVの中心を原点に変換
    float2 uv = input.texcoord - 0.5;

    // グリッド用のワールドスケールに変換
    uv *= 10000.0;

    // 通常グリッド線の設定
    float scale = 1.0;
    float thickness = 1.5;

    float normalLine = gridLine(uv, scale, thickness);
    float gridMask = 1.0 - normalLine;

    // 一定間隔ごとの太線グリッド
    float majorLineThickness = 1.5;
    float majorInterval = 10.0;

    float2 majorUV = uv / majorInterval;
    float majorLine = gridLine(majorUV, 1.0, majorLineThickness);
    float majorMask = 1.0 - majorLine;

    // 太線を優先して合成
    float finalGridMask = max(gridMask, majorMask);

    // 背景とグリッドの基本色
    float3 bgColor = float3(0.05, 0.05, 0.05);
    float3 lineColor = float3(0.07, 0.07, 0.07);
    float3 majorLineColor = float3(0.20, 0.20, 0.20);

    // 通常線と太線をブレンド
    float3 col = lerp(bgColor, lineColor, gridMask);
    col = lerp(col, majorLineColor, majorMask);

    // 原点軸の強調表示
    float axisThickness = 2.0;

    // Z軸を表示
    float zAxis =
        smoothstep(0.0, 1.0,
            abs(uv.x) / (fwidth(uv.x) * axisThickness));
    col = lerp(col, float3(0.1, 0.6, 0.1), 1.0 - zAxis);

    // X軸を表示
    float xAxis =
        smoothstep(0.0, 1.0,
            abs(uv.y) / (fwidth(uv.y) * axisThickness));
    col = lerp(col, float3(0.6, 0.1, 0.1), 1.0 - xAxis);

    return col;
}

bool ShouldDiscardArtGrid(PixelShaderInput input)
{
    float2 uv = (input.texcoord - 0.5f) * 10000.0;
    float zAxis = smoothstep(0.0, 1.0, abs(uv.x) / (fwidth(uv.x) * 2.0));
    float xAxis = smoothstep(0.0, 1.0, abs(uv.y) / (fwidth(uv.y) * 2.0));

    float normalLine = gridLine(uv, 1.0, 2.0);
    float gridMask = 1.0 - normalLine;

    float majorLine = gridLine(uv / 10.0, 1.0, 2.0);
    float majorMask = 1.0 - majorLine;

    float gridAlpha = max(gridMask, majorMask);
    gridAlpha = max(gridAlpha, 1.0 - zAxis);
    gridAlpha = max(gridAlpha, 1.0 - xAxis);

    return gridAlpha < 1e-8;
}

float3 ApplyDirectionalLights(float3 baseColor, float3 pbrAlbedo, float3 normal, float3 toEye, float shadowFactor)
{
    float3 finalColor = float3(0.0f, 0.0f, 0.0f);

    float roughness = clamp(gMaterial.roughness, 0.05f, 1.0f);
    float metalness = saturate(gMaterial.metalness);

    for (int i = 0; i < MAX_DIRECTIONAL_LIGHTS; ++i)
    {
        if (gDirectionalLights[i].enable == 0)
            continue;

        float3 lightDir = normalize(-gDirectionalLights[i].direction);
        float3 lightColor = gDirectionalLights[i].color.rgb * gDirectionalLights[i].color.a;
        float lightIntensity = gDirectionalLights[i].intensity;

        float NdotL = dot(normal, lightDir);
        float saturateNdotL = saturate(NdotL);

        // 自己陰(NdotL)と落ち影(shadowFactor)を合わせた明るさ
        float combinedShadow = saturateNdotL;
        if (i == 0)
            combinedShadow *= shadowFactor;

        float3 radiance = float3(0.0f, 0.0f, 0.0f);

        if (gMaterial.lightMode == SHADING_MODEL_PBR)
        {
            radiance = CalculatePBR(pbrAlbedo, normal, toEye, lightDir, lightColor, lightIntensity, roughness, metalness);
            if (i == 0)
                radiance *= shadowFactor; // 影の濃さが適用済みの数値をそのまま掛ける
        }
        else
        {
            float3 diffuse = float3(0.0f, 0.0f, 0.0f);
            float3 specular = float3(0.0f, 0.0f, 0.0f);

            if (gMaterial.lightMode == SHADING_MODEL_HALFLAMBERT)
            {
                float halfLambert = pow(saturateNdotL * 0.5f + 0.5f, gMaterial.diffuseReflection);
                if (i == 0)
                    halfLambert *= shadowFactor;

                diffuse = gMaterial.color.rgb * baseColor * lightColor * halfLambert * lightIntensity;
            }
            else if (gMaterial.lightMode == SHADING_MODEL_PHONG)
            {
                diffuse = gMaterial.color.rgb * baseColor * lightColor * combinedShadow * lightIntensity;

                if (NdotL > 0.0f)
                {
                    float3 halfVec = normalize(lightDir + toEye);
                    float spec = pow(saturate(dot(normal, halfVec)), gMaterial.shininess);
                    specular = gMaterial.specularColor.rgb * lightColor * spec * lightIntensity;
                    if (i == 0)
                        specular *= shadowFactor;
                }
            }
            else if (gMaterial.lightMode == SHADING_MODEL_TOON)
            {
                float rampU = NdotL * 0.5f + 0.5f;
                if (i == 0)
                    rampU *= shadowFactor; // ランプUVに直接適用
                
                float3 rampColor = gToonRamp.Sample(gClampSampler, float2(rampU, 0.5f)).rgb;
                diffuse = gMaterial.color.rgb * baseColor * rampColor * lightColor * lightIntensity;
            }

            radiance = diffuse + specular;
        }

        finalColor += radiance;
    }

    return finalColor;
}

float3 ApplyPointLights(float3 baseColor, float3 pbrAlbedo, float3 normal, float3 worldPos, float3 toEye)
{
    float3 finalColor = float3(0.0f, 0.0f, 0.0f);

    float roughness = clamp(gMaterial.roughness, 0.05f, 1.0f);
    float metalness = saturate(gMaterial.metalness);

    for (int i = 0; i < MAX_POINT_LIGHTS; ++i)
    {
        if (gPointLights[i].enable == 0)
            continue;
        
        // ベクトル計算
        float3 lightVec = gPointLights[i].position - worldPos;
        float distance = length(lightVec);
        float3 lightDir = normalize(lightVec); // 光源へのベクトル

        // 距離減衰
        float attenuation = gPointLights[i].radius > 0.001f
            ? pow(saturate(1.0f - distance / gPointLights[i].radius), gPointLights[i].decay)
            : 1.0f;

        float3 lightColor = gPointLights[i].color.rgb;
        float lightIntensity = gPointLights[i].intensity;

        float3 radiance = float3(0.0f, 0.0f, 0.0f);

        if (gMaterial.lightMode == SHADING_MODEL_PBR)
        {
            // PBR
            float3 pbrResult = CalculatePBR(pbrAlbedo, normal, toEye, lightDir, lightColor, lightIntensity, roughness, metalness);
            radiance = pbrResult * attenuation;
        }
        else
        {
            // Legacy
            float ndotl = saturate(dot(normal, lightDir));
            float3 diffuse = gMaterial.color.rgb * baseColor * lightColor * ndotl * lightIntensity * attenuation;
            
            float3 specular = float3(0, 0, 0);
            if (ndotl > 0.0f)
            {
                float3 halfVec = normalize(lightDir + toEye);
                float spec = pow(saturate(dot(normal, halfVec)), gMaterial.shininess);
                specular = gMaterial.specularColor.rgb * lightColor * lightIntensity * spec * attenuation;
            }
            radiance = diffuse + specular;
        }

        finalColor += radiance;
    }

    return finalColor;
}

float3 ApplySpotLights(float3 baseColor, float3 pbrAlbedo, float3 normal, float3 worldPos, float3 toEye)
{
    float3 finalColor = float3(0.0f, 0.0f, 0.0f);

    float roughness = clamp(gMaterial.roughness, 0.05f, 1.0f);
    float metalness = saturate(gMaterial.metalness);

    for (int i = 0; i < MAX_SPOT_LIGHTS; ++i)
    {
        if (gSpotLights[i].enable == 0)
            continue;
        
        // ベクトル計算
        float3 lightVecFromLight = worldPos - gSpotLights[i].position;
        float distance = length(lightVecFromLight);
        float3 dirFromLight = normalize(lightVecFromLight); // 光源からサーフェイスへの向き

        // 距離減衰
        float distanceAtt = gSpotLights[i].distance > 0.0001f
            ? pow(saturate(1.0f - distance / gSpotLights[i].distance), gSpotLights[i].decay)
            : 1.0f;

        // 角度減衰
        float coneDot = dot(normalize(gSpotLights[i].direction), dirFromLight);
        float angleAtt = (coneDot > gSpotLights[i].cosAngle)
            ? pow(saturate((coneDot - gSpotLights[i].cosAngle) / (1.0f - gSpotLights[i].cosAngle)), 2.0f)
            : 0.0f;

        float attenuation = distanceAtt * angleAtt;
        float3 lightColor = gSpotLights[i].color.rgb;
        float lightIntensity = gSpotLights[i].intensity;

        // ライティング計算用ベクトル (サーフェイスから光源へ向かうベクトルL)
        float3 lightDirL = -dirFromLight;

        float3 radiance = float3(0.0f, 0.0f, 0.0f);

        if (gMaterial.lightMode == SHADING_MODEL_PBR)
        {
            // PBR
            float3 pbrResult = CalculatePBR(pbrAlbedo, normal, toEye, lightDirL, lightColor, lightIntensity, roughness, metalness);
            radiance = pbrResult * attenuation;
        }
        else
        {
            // Legacy
            float ndotl = saturate(dot(normal, lightDirL));
            float3 diffuse = gMaterial.color.rgb * baseColor * lightColor * ndotl * lightIntensity * attenuation;
            
            float3 specular = float3(0, 0, 0);
            if (ndotl > 0.0f)
            {
                float3 halfVec = normalize(lightDirL + toEye);
                float spec = pow(saturate(dot(normal, halfVec)), gMaterial.shininess);
                specular = gMaterial.specularColor.rgb * lightColor * lightIntensity * spec * attenuation;
            }
            radiance = diffuse + specular;
        }

        finalColor += radiance;
    }

    return finalColor;
}

float3 ApplyAreaLights(float3 baseColor, float3 pbrAlbedo, float3 normal, float3 worldPos, float3 toEye)
{
    float3 finalColor = float3(0.0f, 0.0f, 0.0f);

    float roughness = clamp(gMaterial.roughness, 0.05f, 1.0f);
    float metalness = saturate(gMaterial.metalness);

    for (int i = 0; i < MAX_AREA_LIGHTS; ++i)
    {
        if (gAreaLights[i].enable == 0)
            continue;

        // 代表点近似 (Representative Point)
        float3 vecToPixel = worldPos - gAreaLights[i].position;
        float3 rightDir = normalize(gAreaLights[i].right);
        float3 upDir = normalize(gAreaLights[i].up);
        float halfWidth = length(gAreaLights[i].right);
        float halfHeight = length(gAreaLights[i].up);

        float projRight = dot(vecToPixel, rightDir);
        float projUp = dot(vecToPixel, upDir);
        float clampedRight = clamp(projRight, -halfWidth, halfWidth);
        float clampedUp = clamp(projUp, -halfHeight, halfHeight);

        float3 closestPointOnLight = gAreaLights[i].position + rightDir * clampedRight + upDir * clampedUp;

        // ベクトル計算
        float3 lightVec = closestPointOnLight - worldPos;
        float distance = length(lightVec);
        float3 lightDir = normalize(lightVec); // L

        // 減衰
        float attenuation = gAreaLights[i].range > 0.001f
            ? pow(saturate(1.0f - distance / gAreaLights[i].range), gAreaLights[i].decay)
            : 1.0f;

        float3 lightColor = gAreaLights[i].color.rgb;
        float lightIntensity = gAreaLights[i].intensity;

        float3 radiance = float3(0.0f, 0.0f, 0.0f);

        if (gMaterial.lightMode == SHADING_MODEL_PBR)
        {
            // PBR
            float3 pbrResult = CalculatePBR(pbrAlbedo, normal, toEye, lightDir, lightColor, lightIntensity, roughness, metalness);
            radiance = pbrResult * attenuation;
        }
        else
        {
            // Legacy
            float ndotl = saturate(dot(normal, lightDir));
            float3 diffuse = gMaterial.color.rgb * baseColor * lightColor * ndotl * lightIntensity * attenuation;
            
            float3 specular = float3(0, 0, 0);
            if (ndotl > 0.0f)
            {
                float3 halfVec = normalize(lightDir + toEye);
                float spec = pow(saturate(dot(normal, halfVec)), gMaterial.shininess);
                specular = gMaterial.specularColor.rgb * lightColor * lightIntensity * spec * attenuation;
            }
            radiance = diffuse + specular;
        }

        finalColor += radiance;
    }

    return finalColor;
}

static const float2 poissonDisk[16] =
{
    float2(-0.94201624, -0.39906216), float2(0.94558609, -0.76890725),
    float2(-0.094184101, -0.92938870), float2(0.34495938, 0.29387760),
    float2(-0.91588581, 0.45771432), float2(-0.81544232, -0.87912464),
    float2(-0.38277543, 0.27676845), float2(0.97484398, 0.75648379),
    float2(0.44323325, -0.97511554), float2(0.53742981, -0.47373420),
    float2(-0.26496911, -0.41893023), float2(0.79197514, 0.19090188),
    float2(-0.24188840, 0.99706507), float2(-0.81409955, 0.91437590),
    float2(0.19984126, 0.78641367), float2(0.14383161, -0.14100790)
};

// シャドウ強度を計算
float CalculateShadow(float4 shadowCoord, float3 normal)
{
    float3 projCoords = shadowCoord.xyz / shadowCoord.w;

    // ライトの方向と法線の内積（N dot L）を計算
    float3 lightDir = normalize(-gDirectionalLights[0].direction);
    float NdotL = dot(normal, lightDir);

    // 影の最低値（最も暗い状態）
    float minShadow = 1.0f - saturate(gMaterial.shadowDensity);

    // 光源から見て完全に裏側（NdotLが0以下）なら、影に
    if (NdotL <= 0.0f)
    {
        return minShadow;
    }

    // 法線ベースのバイアス
    float biasScale = saturate(1.0f - NdotL);
    float depthBias = gMaterial.shadowBias;
    float normalBias = 0.002f * biasScale;

    // NDC→UV
    projCoords.x = projCoords.x * 0.5f + 0.5f;
    projCoords.y = -projCoords.y * 0.5f + 0.5f;

    // 法線オフセット
    projCoords.xy += normal.xy * normalBias;

    float currentDepth = projCoords.z - depthBias;

    // 範囲外のクリッピング処理
    if (projCoords.z < 0.0f || projCoords.z > 1.0f ||
        projCoords.x < 0.0f || projCoords.x > 1.0f ||
        projCoords.y < 0.0f || projCoords.y > 1.0f)
    {
        return 1.0f;
    }

    // PCFによる柔らかさの計算
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

    // 平均化
    float shadowVisibility = shadow * (1.0f / 16.0f);
    
    // minShadow ～ 1.0 の範囲に変換して返す
    return lerp(minShadow, 1.0f, shadowVisibility);
}

float3 ApplyRimLight(float3 normal, float3 toEye, float3 toLight)
{
    // 基本のリムライト
    float NdotV = saturate(dot(normal, toEye));
    float rim = 1.0f - NdotV;
    rim = pow(rim, max(gMaterial.rimPower, 0.001f));

    // ライト方向によるマスク処理
    if (gMaterial.rimUseLightDir != 0)
    {
        // ライトが当たっている面 (NdotL) の強さを掛ける
        float NdotL = saturate(dot(normal, toLight));
        rim *= NdotL;
    }

    return gMaterial.rimColor * rim * gMaterial.rimIntensity;
}

float3 CalculateNormalFromMap(PixelShaderInput input, float3 normal, float2 uv)
{
    // UVタイリング補正
    float2 tiledUV = uv * gMaterial.normalTiling;

    // ノーマルマップから法線をサンプリング
    float3 mapSample = gNormalTexture.Sample(gSampler, tiledUV);
    float3 mapNormal = mapSample;
    
    // (0,1)を(-1,1)に変換
    mapNormal = mapNormal * 2.0f - 1.0f;

    // 法線の強度調整
    mapNormal.xy *= gMaterial.normalIntensity;

    // BN行列の構築と変換
    float3 N = normalize(normal);
    // グラム・シュミットの直交化
    float3 T = normalize(input.tangent - dot(input.tangent, N) * N);
    float3 B = cross(N, T);

    float3x3 TBN = float3x3(T, B, N);
    float3 transformedNormal = mul(mapNormal, TBN);

    return normalize(transformedNormal);
}


// Fresnel(角度による反射率の変化)
// F0: 正面から見たときの反射率（金属ならAlbedo、非金属なら0.04）
float3 F_Schlick(float cosTheta, float3 F0)
{
    return F0 + (1.0f - F0) * pow(clamp(1.0f - cosTheta, 0.0f, 1.0f), 5.0f);
}

// Distribution(ハイライトの形状と強さ)
// N: 法線, H: ハーフベクトル, roughness: 粗さ
float D_GGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0f);
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
    denom = PI * denom * denom;

    return nom / max(denom, EPSILON);
}

// Geometry表面の微細な凹凸による遮蔽
// N: 法線, V: 視線, L: ライト方向, roughness: 粗さ
float G_SchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0f);
    float k = (r * r) / 8.0f;

    float nom = NdotV;
    float denom = NdotV * (1.0f - k) + k;

    return nom / max(denom, EPSILON);
}

float G_Smith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0f);
    float NdotL = max(dot(N, L), 0.0f);
    float ggx1 = G_SchlickGGX(NdotV, roughness);
    float ggx2 = G_SchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

// 単一のライトに対するPBR計算
float3 CalculatePBR(
    float3 albedo,
    float3 N,
    float3 V,
    float3 L,
    float3 lightColor,
    float lightIntensity,
    float roughness,
    float metalness
)
{
    float3 H = normalize(V + L); // ハーフベクトル

    // PBRパラメータの準備
    float3 F0 = float3(0.04f, 0.04f, 0.04f);
    F0 = lerp(F0, albedo, metalness);

    // BRDF項の計算
    float NDF = D_GGX(N, H, roughness);
    float G = G_Smith(N, V, L, roughness);
    float3 F = F_Schlick(max(dot(H, V), 0.0f), F0);
       
    // スペキュラの計算
    float3 numerator = NDF * G * F;
    float NdotL = max(dot(N, L), 0.0f);
    float NdotV = max(dot(N, V), 0.0f);
    float denominator = 4.0f * NdotV * NdotL + 0.0001f;
    float3 specular = numerator / denominator;
    
    // エネルギー保存則
    float3 kS = F;
    float3 kD = float3(1.0f, 1.0f, 1.0f) - kS;
    
    // 金属は拡散反射を持たない
    kD *= 1.0f - metalness;

    // 最終合成
    return (kD * albedo / PI + specular) * lightColor * lightIntensity * NdotL;
}

// カラーテクスチャ用トライプラナーマッピング
float4 CalculateTriplanarColor(float3 worldPos, float3 worldNormal, float texScale, float blendSharpness)
{
    // 各軸のブレンド割合を計算
    float3 blendWeights = abs(worldNormal);
    blendWeights = pow(blendWeights, blendSharpness);
    blendWeights /= max(blendWeights.x + blendWeights.y + blendWeights.z, 0.0001f); // 0除算防止

    // 3方向のUVを計算
    float2 uvX = worldPos.zy * texScale;
    float2 uvY = worldPos.xz * texScale;
    float2 uvZ = worldPos.xy * texScale;

    // 3方向からサンプリング
    float4 tX = gTexture.Sample(gSampler, uvX);
    float4 tY = gTexture.Sample(gSampler, uvY);
    float4 tZ = gTexture.Sample(gSampler, uvZ);

    // ウェイトに基づいて合成
    return tX * blendWeights.x + tY * blendWeights.y + tZ * blendWeights.z;
}

// ノーマルマップ用トライプラナーマッピング
float3 CalculateTriplanarNormal(float3 worldPos, float3 worldNormal, float texScale, float blendSharpness)
{
    float3 blendWeights = abs(worldNormal);
    blendWeights = pow(blendWeights, blendSharpness);
    blendWeights /= max(blendWeights.x + blendWeights.y + blendWeights.z, 0.0001f);

    float2 uvX = worldPos.zy * texScale;
    float2 uvY = worldPos.xz * texScale;
    float2 uvZ = worldPos.xy * texScale;

    float3 tX = gNormalTexture.Sample(gSampler, uvX).xyz * 2.0f - 1.0f;
    float3 tY = gNormalTexture.Sample(gSampler, uvY).xyz * 2.0f - 1.0f;
    float3 tZ = gNormalTexture.Sample(gSampler, uvZ).xyz * 2.0f - 1.0f;

    // ワールド空間の向きに合わせてアンパック
    float3 nX = float3(tX.z * sign(worldNormal.x), tX.y, -tX.x);
    float3 nY = float3(tY.x, tY.z * sign(worldNormal.y), -tY.y);
    float3 nZ = float3(tZ.x, tZ.y, tZ.z * sign(worldNormal.z));

    float3 finalNormal = nX * blendWeights.x + nY * blendWeights.y + nZ * blendWeights.z;

    return normalize(finalNormal);
}