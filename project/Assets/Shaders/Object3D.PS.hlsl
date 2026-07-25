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
ConstantBuffer<ShadowData> gShadowData : register(b8);

Texture2D<float4> gTexture : register(t0);
TextureCube<float4> gEnvironmentTexture : register(t1);
Texture2DArray<float> gShadowMapArray : register(t2);
Texture2D<float4> gToonRamp : register(t3);
Texture2D<float4> gDissolveTexture : register(t4);
Texture2D<float3> gNormalTexture : register(t5);
Texture2D<float3> gRippleTexture : register(t6);
Texture2D<float> gPuddleNoiseTexture : register(t7);
Texture2D<float> gPOMHeightMap : register(t8);

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

float D_GGX(float3 N, float3 H, float roughness);
float G_SchlickGGX(float NdotV, float roughness);
float G_Smith(float3 N, float3 V, float3 L, float roughness);
float3 F_SchlickRoughness(float cosTheta, float3 F0, float roughness);

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
float CalculateShadowCSM(float3 worldPos, float3 normal, float viewDepth);
float3 CalculateNormalFromMap(PixelShaderInput input, float3 normal, float2 uv);
float3 CalculateTriplanarNormal(float3 worldPos, float3 worldNormal, float texScale, float blendSharpness);
float4 CalculateTriplanarColor(float3 worldPos, float3 worldNormal, float texScale, float blendSharpness);

float2 CalculateParallaxOcclusionMapping(float2 texCoords, float3 viewDirTS, float2 dx, float2 dy, out float parallaxHeight);
float CalculatePOMSoftShadow(float3 lightDirTS, float2 initialUV, float initialHeight, float2 dx, float2 dy);

PixelShaderOutput main(PixelShaderInput input)
{
    PixelShaderOutput output;

    // カメラへの視線ベクトル (ワールド空間)
    float3 toEyeWorld = normalize(gFrameData.cameraWorldPosition - input.worldPosition);

    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float2 finalUV = transformedUV.xy;

    // --------------------------------------------------------
    // POM (Parallax Occlusion Mapping) の適用
    // --------------------------------------------------------
    float pomSelfShadow = 1.0f;
    
    // トライプラナーマッピング時はPOMの計算が極めて重く複雑になるため除外する設計が一般的です
    if (gMaterial.enablePOM != 0 && gMaterial.useTriplanar == 0)
    {
        // 1. 接空間(Tangent Space)を構築するためのTBN行列を作成
        // ※理想は頂点シェーダーから Tangent/Binormal を渡すことですが、
        // ない場合は ddx/ddy を用いて擬似的に接空間を構築します。
        // （入力に tangent がある場合は入力値を使用してください）
        float3 N = normalize(input.normal);
        float3 T = normalize(input.tangent);
// B(従法線)はNとTの外積で求められます
        float3 B = normalize(cross(N, T)); // ※エンジンによっては cross(T, N) になる場合があります

        float3x3 TBN = float3x3(T, B, N);

        // ワールド空間の視線ベクトルを接空間に変換
        float3 toEyeTS = mul(TBN, toEyeWorld);
        
        // SampleGrad用の微小変化量をループ外で取得 (必須)
        float2 dx = ddx(finalUV);
        float2 dy = ddy(finalUV);

        float parallaxHeight = 0.0f;
        // UV座標を視差に応じてオフセット
        finalUV = CalculateParallaxOcclusionMapping(finalUV, toEyeTS, dx, dy, parallaxHeight);

        // UVが0〜1の範囲外に出た場合はDiscardする処理 (オプション。エッジの切れ目対策)
        // if (finalUV.x < 0.0 || finalUV.x > 1.0 || finalUV.y < 0.0 || finalUV.y > 1.0) discard;

        // 自己シャドウの計算 (DirectionalLight[0] がある場合)
        if (gDirectionalLights[0].enable != 0)
        {
            float3 lightDirWorld = normalize(-gDirectionalLights[0].direction);
            float3 lightDirTS = mul(TBN, lightDirWorld);
            pomSelfShadow = CalculatePOMSoftShadow(lightDirTS, finalUV, parallaxHeight, dx, dy);
        }
    }
    
    // ベースとなるワールド法線を計算
    float3 worldNormal = normalize(input.normal);
    
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
        textureColor = gTexture.Sample(gSampler, finalUV);
    }
    
        
    // ディザー透明処理
    if (textureColor.a * gMaterial.color.a <= gMaterial.alphaTestThreshold)
    {
        discard; // これ以降の重いライティング計算をスキップ
    }
    
    float3 baseColor = textureColor.rgb;

    // ディゾルブ処理
    float3 dissolveEdgeEmission = float3(0, 0, 0);

    // マテリアル設定でディゾルブが有効、かつ閾値が0より大きい場合のみ計算
    if (gMaterial.enableDissolve != 0)
    {
        // ノイズテクスチャをサンプリング
        float noiseValue = gDissolveTexture.Sample(gSampler, finalUV).r;

        // ノイズの値が閾値より低ければピクセルを捨てる
        if (noiseValue <= gMaterial.dissolveThreshold)
        {
            discard;
        }

        // 境界線の発光
        float difference = noiseValue - gMaterial.dissolveThreshold;
        
        // エッジ幅の範囲内なら発光
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
        output.normal = float4(0.0f, 1.0f, 0.0f, 1.0f);
        output.material = float4(0.0f, 1.0f, 0.0f, 1.0f);
        
        return output;
    }
    
    // 影の計算への適用
    float shadowFactor = 1.0f;
    if (gDirectionalLights[0].enable && gMaterial.addShadow != 0)
    {
        float viewDepth = distance(gFrameData.cameraWorldPosition, input.worldPosition);
        shadowFactor = CalculateShadowCSM(input.worldPosition, normalize(input.normal), viewDepth);
        
        // CSMの影にPOMの自己影を合成する (最も暗い方を採用)
        shadowFactor = min(shadowFactor, pomSelfShadow);
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
            normal = CalculateNormalFromMap(input, worldNormal, finalUV);
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
            float3 kS = F_SchlickRoughness(max(dot(normal, toEye), 0.0f), float3(0.04f, 0.04f, 0.04f), currentRoughness);
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
            float3 F_env = F_SchlickRoughness(max(dot(normal, toEye), 0.0f), F0, currentRoughness);
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
    
    // G-Bufferへの情報書き込み

    // 法線情報
    output.normal = float4(normal, 1.0f);

    // 材質情報
    // R=メタルネス, G=ラフネス
    output.material = float4(currentMetalness, currentRoughness, 0.0f, 1.0f);
    
    // モーションベクトルの計算
    // W除算を行ってNDC空間（-1 ～ 1）へ変換
    float2 currentNDC = input.currentClipPos.xy / input.currentClipPos.w;
    float2 prevNDC = input.prevClipPos.xy / input.prevClipPos.w;

    // NDCからUV空間（0 ～ 1）へ変換 (Y軸の反転に注意)
    float2 currentUV = currentNDC * float2(0.5f, -0.5f) + 0.5f;
    float2 prevUV = prevNDC * float2(0.5f, -0.5f) + 0.5f;

    // 移動量の算出（現在のUV - 1フレーム前のUV）
    output.velocity = currentUV - prevUV;
    
    // TAAのカメラジッター（微細なズレ）を入れている場合、
    // ここで計算する行列からはジッターを抜いておくか、Velocityからジッター分を引く必要がある
    
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
        
        float3 lightVec = gPointLights[i].position - worldPos;
        float distance = length(lightVec);
        float radius = gPointLights[i].radius;
        
        if (distance > radius)
            continue;

        float3 lightDir = (distance > 0.001f) ? (lightVec / distance) : float3(0.0f, 1.0f, 0.0f);
        
        // 数学的減衰（powベース）
        float decay = 2.0f; 
        float attenuation = pow(saturate(1.0f - distance / radius), decay);

        float3 lightColor = gPointLights[i].color.rgb;
        float3 radiance = float3(0.0f, 0.0f, 0.0f);

        if (gMaterial.lightMode == SHADING_MODEL_PBR)
        {
            // PBR
            radiance = CalculatePBR(pbrAlbedo, normal, toEye, lightDir, lightColor, gPointLights[i].intensity, roughness, metalness) * attenuation;
        }
        else
        {
            // Legacy
            float ndotl = saturate(dot(normal, lightDir));
            float3 diffuse = gMaterial.color.rgb * baseColor * lightColor * ndotl * gPointLights[i].intensity * attenuation;
            
            float3 specular = float3(0, 0, 0);
            if (ndotl > 0.0f)
            {
                float3 halfVec = normalize(lightDir + toEye);
                float spec = pow(saturate(dot(normal, halfVec)), gMaterial.shininess);
                specular = gMaterial.specularColor.rgb * lightColor * gPointLights[i].intensity * spec * attenuation;
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
        
        float3 lightVecFromLight = worldPos - gSpotLights[i].position;
        float distance = length(lightVecFromLight);
        
        if (distance > gSpotLights[i].distance)
            continue;

        float3 dirFromLight = (distance > 0.001f) ? (lightVecFromLight / distance) : normalize(gSpotLights[i].direction);
        
        // 距離減衰
        float distanceRatio = distance / gSpotLights[i].distance;
        float distanceAtt = pow(saturate(1.0f - distanceRatio), 2.0f);
        
        // 角度減衰
        float coneDot = dot(normalize(gSpotLights[i].direction), dirFromLight);
        float cosOuter = gSpotLights[i].cosAngle;
        float cosInner = lerp(1.0f, cosOuter, 0.8f);
        
        // smoothstep を使った滑らかな角度減衰
        float angleAtt = smoothstep(cosOuter, cosInner, coneDot);

        float attenuation = distanceAtt * angleAtt;
        if (attenuation <= 0.0f)
            continue;

        float3 lightColor = gSpotLights[i].color.rgb;
        float lightIntensity = gSpotLights[i].intensity;
        float3 lightDirL = -dirFromLight;
        float3 radiance = float3(0.0f, 0.0f, 0.0f);

        if (gMaterial.lightMode == SHADING_MODEL_PBR)
        {
            // PBR用ハイライトのエリアライト化
            float3 R = reflect(-toEye, normal);
            float fakeSourceRadius = 0.1f;
            float3 closestPoint = lightDirL + R * clamp(dot(lightDirL, R), 0.0f, fakeSourceRadius);
            float3 modifiedLightDirL = normalize(closestPoint);
            
            float alpha = roughness * roughness;
            float alphaPrime = saturate(alpha + (fakeSourceRadius / max(distance, 0.001f) * 0.5f));
            float modifiedRoughness = sqrt(alphaPrime);

            float3 pbrResult = CalculatePBR(pbrAlbedo, normal, toEye, modifiedLightDirL, lightColor, lightIntensity, modifiedRoughness, metalness);
            radiance = pbrResult * attenuation;
        }
        else
        {
            // Legacy (Blinn-Phong)
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

// 特定の1つのカスケードから影の濃さを取得するヘルパー関数
float SampleSingleCascade(float3 worldPos, float3 normal, uint cascadeIndex)
{
    float3 lightDir = normalize(-gDirectionalLights[0].direction);
    float NdotL = dot(normal, lightDir);
    
    // バイアス計算
    float biasScale = saturate(1.0f - NdotL);
    float worldNormalBias = gMaterial.shadowNormalBias * biasScale;
    float3 biasedWorldPos = worldPos + normal * worldNormalBias;

    // ライト空間への変換
    float4 shadowCoord = mul(float4(biasedWorldPos, 1.0f), gShadowData.cascadeLightViewProj[cascadeIndex]);
    float3 projCoords = shadowCoord.xyz / shadowCoord.w;

    projCoords.x = projCoords.x * 0.5f + 0.5f;
    projCoords.y = -projCoords.y * 0.5f + 0.5f;

    float currentDepth = projCoords.z - gMaterial.shadowBias;

    // 範囲外判定
    if (projCoords.z < 0.0f || projCoords.z > 1.0f ||
        projCoords.x < 0.0f || projCoords.x > 1.0f ||
        projCoords.y < 0.0f || projCoords.y > 1.0f)
    {
        return 1.0f; // 影なし
    }

    float2 texelSize = 1.0f / 2048.0f;
    float softness = max(gMaterial.shadowSoftness, 1.0f);
    float shadow = 0.0f;

    // 16回のPCFサンプリング
    [unroll]
    for (int i = 0; i < 16; ++i)
    {
        float2 offset = poissonDisk[i] * texelSize * softness;
        shadow += gShadowMapArray.SampleCmpLevelZero(
            gShadowSampler,
            float3(projCoords.xy + offset, cascadeIndex),
            currentDepth
        );
    }

    return shadow * (1.0f / 16.0f);
}

// シャドウ強度を計算
float CalculateShadowCSM(float3 worldPos, float3 normal, float viewDepth)
{
    float3 lightDir = normalize(-gDirectionalLights[0].direction);
    float NdotL = dot(normal, lightDir);
    float minShadow = 1.0f - saturate(gMaterial.shadowDensity);

    if (NdotL <= 0.0f)
        return minShadow;

    uint cascadeIndex = 0;
    float nextSplitDist = 0.0f;

    // どのカスケードに属しているか判定しつつ、次の境界線の距離も取得
    if (viewDepth > gShadowData.cascadeSplits.z)
    {
        cascadeIndex = 3;
        nextSplitDist = 999999.0f; // これ以上奥はないのでブレンドしない
    }
    else if (viewDepth > gShadowData.cascadeSplits.y)
    {
        cascadeIndex = 2;
        nextSplitDist = gShadowData.cascadeSplits.z;
    }
    else if (viewDepth > gShadowData.cascadeSplits.x)
    {
        cascadeIndex = 1;
        nextSplitDist = gShadowData.cascadeSplits.y;
    }
    else
    {
        cascadeIndex = 0;
        nextSplitDist = gShadowData.cascadeSplits.x;
    }

    // メインとなる現在のカスケードから影を取得
    float shadowVisibility = SampleSingleCascade(worldPos, normal, cascadeIndex);

    // カスケードシーム（ブレンド）処理
    // 境界線を跨ぐブレンド幅
    float blendBand = 2.0f;

    // 次の境界線にどれくらい近いかを 0.0 〜 1.0で計算
    float blendFactor = smoothstep(nextSplitDist - blendBand, nextSplitDist, viewDepth);

    // もし境界線付近（0.0より大きい）で、かつ次のカスケードが存在するなら
    if (blendFactor > 0.0f && cascadeIndex < 3)
    {
        // 次のカスケード（荒い影）も取得
        float nextShadowVisibility = SampleSingleCascade(worldPos, normal, cascadeIndex + 1);
        
        // 現在の影と次の影を滑らかにブレンド
        shadowVisibility = lerp(shadowVisibility, nextShadowVisibility, blendFactor);
    }

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

// 粗さを考慮したFresnel
float3 F_SchlickRoughness(float cosTheta, float3 F0, float roughness)
{
    // 粗い材質ほど、最大反射率（F90）を下げる
    float maxReflectance = 1.0f - roughness;
    float3 F90 = max(float3(maxReflectance, maxReflectance, maxReflectance), F0);
    
    return F0 + (F90 - F0) * pow(clamp(1.0f - cosTheta, 0.0f, 1.0f), 5.0f);
}

// 単一のライトに対するPBR計算（Cook-Torrance BRDF）
float3 CalculatePBR(
    float3 albedo,
    float3 N,
    float3 V,
    float3 L,
    float3 lightColor,
    float lightIntensity,
    float roughness,
    float metalness)
{
    float3 H = normalize(V + L); // ハーフベクトル

    // PBRパラメータの準備
    float3 F0 = float3(0.04f, 0.04f, 0.04f);
    F0 = lerp(F0, albedo, metalness);

    // BRDF項の計算
    float NDF = D_GGX(N, H, roughness);
    float G = G_Smith(N, V, L, roughness);
    
    // F_SchlickRoughnessに置き換え、roughnessを渡す
    float3 F = F_SchlickRoughness(max(dot(H, V), 0.0f), F0, roughness);
       
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
    return (kD * albedo / 3.14159265f + specular) * lightColor * lightIntensity * NdotL;
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

float2 CalculateParallaxOcclusionMapping(
    float2 texCoords,
    float3 viewDirTS,
    float2 dx,
    float2 dy,
    out float parallaxHeight)
{
    viewDirTS = normalize(viewDirTS);

    if (viewDirTS.z <= 0.0f)
    {
        parallaxHeight = 0.0f;
        return texCoords;
    }

    float mipLevel = gPOMHeightMap.CalculateLevelOfDetail(gSampler, texCoords);

    // 【防御1】ハイトスケールの強制クランプとUVスケール補正
    // どんなに大きな値が来ても、UV空間上で最大10%（0.1）以上のズレを許容しない。
    // ※ 0.1 でもPOMとしてはかなり深いです。
    float safeHeightScale = clamp(gMaterial.pomHeightScale, 0.0f, 0.1f);
    
    // 【防御2】UVタイリングによるスケーリングを相殺
    // dx, dy の長さからUVがどれくらい縮小/拡大されているかを概算し、
    // タイリングされている場合はPOMのスケールも小さくして破綻を防ぐ
    float uvScale = length(float2(dx.x, dy.y)) * 1024.0f; // 基準サイズに対する倍率
    safeHeightScale /= max(uvScale, 1.0f);

    // 【防御3】スケールに応じた動的ステップ数のブースト
    // 浅ければ少ないループで軽くし、深く設定されたら自動でループを増やして突き抜けを防ぐ
    float scaleFactor = (safeHeightScale / 0.05f); // 0.05を基準(1.0)とする
    float maxSteps = clamp(gMaterial.pomMaxSteps * scaleFactor, 16.0f, 128.0f);
    float minSteps = clamp(gMaterial.pomMinSteps * scaleFactor, 8.0f, 64.0f);

    float numSteps = lerp(maxSteps, minSteps, viewDirTS.z);
    float stepSize = 1.0f / numSteps;

    float2 parallaxDir = viewDirTS.xy / max(viewDirTS.z, 0.01f);
    
    // Max Ratio Clamping
    float maxRatio = 1.5f;
    float currentRatio = length(parallaxDir);
    if (currentRatio > maxRatio)
    {
        parallaxDir *= (maxRatio / currentRatio);
    }

    // 安全なスケール値を使用
    float2 p = parallaxDir * safeHeightScale;
    float2 deltaTexCoords = p * stepSize;

    float referencePlane = 0.0f;
    float2 currentTexCoords = texCoords + (p * referencePlane);
    
    float currentLayerDepth = 0.0f;
    float currentDepthMapValue = 1.0f - gPOMHeightMap.SampleLevel(gSampler, currentTexCoords, mipLevel).r;

    [unroll(128)] // ブーストに合わせてunroll上限を開放 (※エラーが出る場合は手動で固定値を入れてください)
    while (currentLayerDepth < currentDepthMapValue)
    {
        currentTexCoords -= deltaTexCoords;
        currentLayerDepth += stepSize;
        currentDepthMapValue = 1.0f - gPOMHeightMap.SampleLevel(gSampler, currentTexCoords, mipLevel).r;
    }

    float2 prevTexCoords = currentTexCoords + deltaTexCoords;
    float afterDepth = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = (1.0f - gPOMHeightMap.SampleLevel(gSampler, prevTexCoords, mipLevel).r) - currentLayerDepth + stepSize;

    float weight = afterDepth / (afterDepth - beforeDepth);
    float2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0f - weight);

    parallaxHeight = currentLayerDepth - stepSize * (1.0f - weight);

    return finalTexCoords;
}


// POMによるソフト自己影の計算 (メインPOMと全く同じMax Ratio制限を入れる)
float CalculatePOMSoftShadow(float3 lightDirTS, float2 initialUV, float initialHeight, float2 dx, float2 dy)
{
    lightDirTS = normalize(lightDirTS); // 光源ベクトルも念のため正規化

    if (lightDirTS.z <= 0.0f)
        return 0.0f;

    float mipLevel = gPOMHeightMap.CalculateLevelOfDetail(gSampler, initialUV);

    float numSteps = lerp(gMaterial.pomMaxSteps, gMaterial.pomMinSteps, lightDirTS.z);
    float stepSize = 1.0f / numSteps;

    // シャドウ側も同様に最大長を制限する
    float2 parallaxDir = lightDirTS.xy / max(lightDirTS.z, 0.01f);
    float maxRatio = 1.5f;
    float currentRatio = length(parallaxDir);
    if (currentRatio > maxRatio)
    {
        parallaxDir *= (maxRatio / currentRatio);
    }

    float2 p = parallaxDir * gMaterial.pomHeightScale;
    float2 deltaTexCoords = p * stepSize;

    float2 currentTexCoords = initialUV;
    float currentLayerDepth = initialHeight - stepSize;
    float shadowMultiplier = 1.0f;

    [unroll(32)]
    while (currentLayerDepth > 0.0f)
    {
        currentTexCoords += deltaTexCoords;
        float currentDepthMapValue = 1.0f - gPOMHeightMap.SampleLevel(gSampler, currentTexCoords, mipLevel).r;
        
        if (currentDepthMapValue < currentLayerDepth)
        {
            float currentShadow = (currentLayerDepth - currentDepthMapValue) * 4.0f;
            shadowMultiplier = min(shadowMultiplier, 1.0f - currentShadow);
        }
        currentLayerDepth -= stepSize;
    }

    return saturate(shadowMultiplier);
}