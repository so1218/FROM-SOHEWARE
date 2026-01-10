#include "Object3D.hlsli"
#include "ShaderConstants.hlsli"

#define PI 3.1415926535
#define TAU 6.2831853071

ConstantBuffer<MaterialData> gMaterial : register(b0);
cbuffer DirectionalLights : register(b1)
{
    DirectionalLight gDirectionalLights[MAX_DIRECTIONAL_LIGHTS];
};
ConstantBuffer<FrameData> gFrameData : register(b2);
cbuffer PointLights : register(b3)
{
    PointLight gPointLights[MAX_POINT_LIGHTS];
};
cbuffer SpotLights : register(b4)
{
    SpotLight gSpotLights[MAX_SPOT_LIGHTS];
};
cbuffer AreaLightsBuffer : register(b5) 
{
    AreaLight gAreaLights[MAX_AREA_LIGHTS];
}

Texture2D<float4> gTexture : register(t0);
TextureCube<float4> gEnvironmentTexture : register(t1);
Texture2D<float> gShadowMap : register(t2);
Texture2D<float4> gToonRamp : register(t3);
Texture2D<float4> gDissolveTexture : register(t4);

SamplerState gSampler : register(s0);
SamplerComparisonState gShadowSampler : register(s1);
SamplerState gClampSampler : register(s2);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

struct PixelShaderInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
    float3 worldPosition : POSITION1;
    float4 shadowCoord : POSITION2;
};

float DitherThreshold4x4(int2 position);

float3 DrawArtGridColor(PixelShaderInput input);
bool ShouldDiscardArtGrid(PixelShaderInput input);

float3 ApplyDirectionalLights(float3 baseColor, float3 normal, float3 toEye, float shadowFactor);
float3 ApplyPointLights(float3 baseColor, float3 normal, float3 worldPos, float3 toEye);
float3 ApplySpotLights(float3 baseColor, float3 normal, float3 worldPos, float3 toEye);
float3 ApplyAreaLights(float3 baseColor, float3 normal, float3 worldPos, float3 toEye);
float3 ApplyRimLight(float3 normal, float3 toEye, float3 toLight);

// 影の濃さを計算する関数
float CalculateShadow(float4 shadowCoord, float3 normal);

PixelShaderOutput main(PixelShaderInput input)
{
    PixelShaderOutput output;

    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);
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

            // グラデーションを滑らかにする
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
    float3 normal = normalize(input.normal);
    float3 toEye = normalize(gFrameData.cameraWorldPosition - input.worldPosition);
    
    if (gMaterial.enableLighting != 0)
    {
        // Directional Light
        finalColor += ApplyDirectionalLights(baseColor, normal, toEye, shadowFactor);

        // Point Light
        finalColor += ApplyPointLights(baseColor, normal, input.worldPosition, toEye);

        // Spot Light
        finalColor += ApplySpotLights(baseColor, normal, input.worldPosition, toEye);
        
        // Area Light
        finalColor += ApplyAreaLights(baseColor, normal, input.worldPosition, toEye);
        
        // 環境マップ処理

        // toEye はピクセルからカメラへのベクトル
        float3 reflectedVector = reflect(-toEye, normal);
        
        // 環境マップから色をサンプリング
        float4 environmentColor = gEnvironmentTexture.Sample(gSampler, reflectedVector);
        
        // 環境光を最終的な色に加算する
        finalColor += environmentColor.rgb * gMaterial.environmentMapIntensity;
        
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
            float3 rimColor = ApplyRimLight(normal, toEye, toLight);
            
            finalColor += rimColor;
        }
       
    }
    else
    {
        finalColor = baseColor * gMaterial.color.rgb;
    }
    
     // 自己発光を加算
    finalColor *= gMaterial.emissiveIntensity;
    
    // 最後にディゾルブのエッジ発光を加算
    finalColor += dissolveEdgeEmission;

    output.color.rgb = finalColor;
    output.color.a = textureColor.a * gMaterial.color.a;

    // ディザー透明処理
    //{
    //    int2 screenPos = int2(input.position.xy);
    //    float alpha = output.color.a;

    //    float threshold = DitherThreshold4x4(screenPos);

    //    if (alpha < threshold)
    //    {
    //        discard;
    //    }
    //}

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

float3 ApplyDirectionalLights(float3 baseColor, float3 normal, float3 toEye, float shadowFactor)
{
    float3 finalColor = float3(0.0f, 0.0f, 0.0f);

    for (int i = 0; i < MAX_DIRECTIONAL_LIGHTS; ++i)
    {
        if (gDirectionalLights[i].enable == 0)
            continue;

        // ライト方向・強度・色を取得
        float3 lightDir = normalize(-gDirectionalLights[i].direction);
        // ランプ計算用にsaturateしていない生のdot積を取る
        float NdotL_Raw = dot(normal, lightDir);
        float ndotl = saturate(dot(normal, lightDir));
        float3 lightColor = gDirectionalLights[i].color.rgb * gDirectionalLights[i].color.a;
        float lightIntensity = gDirectionalLights[i].intensity;

        float3 diffuse = float3(0.0f, 0.0f, 0.0f);
        float3 specular = float3(0.0f, 0.0f, 0.0f);

        // ライティングモード別計算
        if (gMaterial.lightMode == LIGHT_HALFLAMBERT)
        {
            float halfLambert = pow(ndotl * 0.5f + 0.5f, gMaterial.diffuseReflection);
            diffuse = gMaterial.color.rgb * baseColor * lightColor * halfLambert * lightIntensity;
        }
        else if (gMaterial.lightMode == LIGHT_PHONG_SPECULAR)
        {
            float halfLambert = pow(ndotl * 0.5f + 0.5f, gMaterial.diffuseReflection +2.0f);
            diffuse = gMaterial.color.rgb * baseColor * lightColor * halfLambert * lightIntensity;

            if (ndotl > 0.0f)
            {
                float3 halfVec = normalize(lightDir + toEye);
                float spec = pow(saturate(dot(normal, halfVec)), gMaterial.shininess);
                specular = gMaterial.specularColor.rgb * lightColor * spec * lightIntensity;
            }
        }
        else if (gMaterial.lightMode == LIGHT_TOON)
        {
            // 法線とライトの角度をUV座標に変換
            float rampU = NdotL_Raw * 0.5f + 0.5f;
            
            // テキスチャから色を取得
            float3 rampColor = gToonRamp.Sample(gClampSampler, float2(rampU, 0.5f)).rgb;

            diffuse = gMaterial.color.rgb * baseColor * rampColor * lightColor * lightIntensity;
        }

        // 影を適用（0番目のライトのみ）
        if (i == 0)
        {
            float shadowAtten = 1.0f - gMaterial.shadowDensity; // 影部分の明るさ
            float finalShadow = shadowFactor + shadowAtten * (1.0f - shadowFactor);

            diffuse *= finalShadow;
            specular *= finalShadow;
        }

        // 合計色に加算
        finalColor += diffuse + specular;
    }

    return finalColor;
}

float3 ApplyPointLights(float3 baseColor, float3 normal, float3 worldPos, float3 toEye)
{
    float3 finalColor = float3(0.0f, 0.0f, 0.0f);

    for (int i = 0; i < MAX_POINT_LIGHTS; ++i)
    {
        if (gPointLights[i].enable == 0)
            continue;
        
        float3 lightDir = normalize(gPointLights[i].position - worldPos);
        float distance = length(gPointLights[i].position - worldPos);
        float attenuation = gPointLights[i].radius > 0.001f
            ? pow(saturate(1.0f - distance / gPointLights[i].radius), gPointLights[i].decay)
            : 1.0f;

        float ndotl = saturate(dot(normal, lightDir));
        float3 diffuse = gMaterial.color.rgb * baseColor * gPointLights[i].color.rgb * ndotl * gPointLights[i].intensity * attenuation;
        finalColor += diffuse;

        if (ndotl > 0.0f)
        {
            float3 halfVec = normalize(lightDir + toEye);
            float spec = pow(saturate(dot(normal, halfVec)), gMaterial.shininess);
            float3 specular = gMaterial.specularColor.rgb * gPointLights[i].color.rgb * gPointLights[i].intensity * spec * attenuation;
            finalColor += specular;
        }
    }

    return finalColor;
}
float3 ApplySpotLights(float3 baseColor, float3 normal, float3 worldPos, float3 toEye)
{
    float3 finalColor = float3(0.0f, 0.0f, 0.0f);

    for (int i = 0; i < MAX_SPOT_LIGHTS; ++i)
    {
        if (gSpotLights[i].enable == 0)
            continue;
        
        float3 lightVec = worldPos - gSpotLights[i].position;
        float distance = length(lightVec);
        float3 lightDir = normalize(lightVec);

        float distanceAtt = gSpotLights[i].distance > 0.0001f
            ? pow(saturate(1.0f - distance / gSpotLights[i].distance), gSpotLights[i].decay)
            : 1.0f;

        float coneDot = dot(normalize(gSpotLights[i].direction), -lightDir);
        float angleAtt = (coneDot > gSpotLights[i].cosAngle)
            ? pow(saturate((coneDot - gSpotLights[i].cosAngle) / (1.0f - gSpotLights[i].cosAngle)), 2.0f)
            : 0.0f;

        float attenuation = distanceAtt * angleAtt;

        float ndotl = saturate(dot(normal, -lightDir));
        float3 diffuse = gMaterial.color.rgb * baseColor * gSpotLights[i].color.rgb * ndotl * gSpotLights[i].intensity * attenuation;
        finalColor += diffuse;

        if (ndotl > 0.0f)
        {
            float3 halfVec = normalize(-lightDir + toEye);
            float spec = pow(saturate(dot(normal, halfVec)), gMaterial.shininess);
            float3 specular = gMaterial.specularColor.rgb * gSpotLights[i].color.rgb * gSpotLights[i].intensity * spec * attenuation;
            finalColor += specular;
        }
    }

    return finalColor;
}

float3 ApplyAreaLights(float3 baseColor, float3 normal, float3 worldPos, float3 toEye)
{
    float3 finalColor = float3(0.0f, 0.0f, 0.0f);

    for (int i = 0; i < MAX_AREA_LIGHTS; ++i)
    {
        if (gAreaLights[i].enable == 0)
            continue;

        // ライト中心からピクセルへのベクトル
        float3 vecToPixel = worldPos - gAreaLights[i].position;

        // ライトのローカル軸（right, up）と半サイズ
        float3 rightDir = normalize(gAreaLights[i].right);
        float3 upDir = normalize(gAreaLights[i].up);
        float halfWidth = length(gAreaLights[i].right);
        float halfHeight = length(gAreaLights[i].up);

        // ピクセルをライトのローカル軸へ射影
        float projRight = dot(vecToPixel, rightDir);
        float projUp = dot(vecToPixel, upDir);

        // 射影点を矩形領域内にクランプ
        float clampedRight = clamp(projRight, -halfWidth, halfWidth);
        float clampedUp = clamp(projUp, -halfHeight, halfHeight);

        // ピクセルに最も近いライト面上の点
        float3 closestPointOnLight = gAreaLights[i].position +
                                     rightDir * clampedRight +
                                     upDir * clampedUp;

        // その点を光源として扱う
        float3 lightVec = closestPointOnLight - worldPos;
        float distance = length(lightVec);
        float3 lightDir = normalize(lightVec);

        // 光の減衰
        float attenuation = gAreaLights[i].range > 0.001f
            ? pow(saturate(1.0f - distance / gAreaLights[i].range), gAreaLights[i].decay)
            : 1.0f;

        // ディフューズ
        float ndotl = saturate(dot(normal, lightDir));
        float3 diffuse = gMaterial.color.rgb * baseColor * gAreaLights[i].color.rgb *
                         ndotl * gAreaLights[i].intensity * attenuation;
        finalColor += diffuse;

        // スペキュラ
        if (ndotl > 0.0f)
        {
            float3 halfVec = normalize(lightDir + toEye);
            float spec = pow(saturate(dot(normal, halfVec)), gMaterial.shininess);
            float3 specular = gMaterial.specularColor.rgb * gAreaLights[i].color.rgb *
                              gAreaLights[i].intensity * spec * attenuation;
            finalColor += specular;
        }
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
    float softness = max(softness, 1.0f);

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

    return shadow * (1.0f / 16.0f);
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