#include "Object3D.hlsli"
#include "ShaderConstants.hlsli"

// 定数定義
#define PI 3.1415926535
#define TAU 6.2831853071

ConstantBuffer<MaterialData> gMaterial : register(b0);
Texture2D<float4> gTexture : register(t0);
TextureCube<float4> gEnvironmentTexture : register(t1);
SamplerState gSampler : register(s0);
Texture2D<float> gShadowMap : register(t2);
SamplerComparisonState gShadowSampler : register(s1);
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

float3 DrawArtWaveColor(PixelShaderInput input);
float3 DrawArtQuadColor(PixelShaderInput input);
float3 DrawArtKikagakuColor(PixelShaderInput input);
float3 DrawArtSoundColor(PixelShaderInput input);
float3 DrawArtFragColor(PixelShaderInput input);
float3 DrawArtGridColor(PixelShaderInput input);
bool ShouldDiscardArtGrid(PixelShaderInput input);

float3 ApplyDirectionalLights(float3 baseColor, float3 normal, float3 toEye, float shadowFactor);
float3 ApplyPointLights(float3 baseColor, float3 normal, float3 worldPos, float3 toEye);
float3 ApplySpotLights(float3 baseColor, float3 normal, float3 worldPos, float3 toEye);
float3 ApplyAreaLights(float3 baseColor, float3 normal, float3 worldPos, float3 toEye);

// 影の濃さを計算する関数
float CalculateShadow(float4 shadowCoord);

PixelShaderOutput main(PixelShaderInput input)
{
    PixelShaderOutput output;

    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);
    float3 baseColor = textureColor.rgb;

    // アートエフェクト適用
    if (gMaterial.isArtWave)
    {
        baseColor = DrawArtWaveColor(input);
    }
    else if (gMaterial.isArtQuad)
    {
        baseColor = DrawArtQuadColor(input);
    }
    else if (gMaterial.isArtKikagaku)
    {
        baseColor = DrawArtKikagakuColor(input);
    }
    else if (gMaterial.isArtSound)
    {
        baseColor = DrawArtSoundColor(input);
    }
    else if (gMaterial.isArtFrag)
    {
        baseColor = DrawArtFragColor(input);
    }
    else if (gMaterial.isArtGrid)
    {
        if (ShouldDiscardArtGrid(input))
        {
            discard;
        }

        output.color.rgb = DrawArtGridColor(input);
        output.color.a = 1.0;
        return output;
    }
    
    // ▼▼▼ 影の計算 ▼▼▼
    float shadowFactor = 1.0f;
    
    // 0番目のライトが有効なら影を計算
    if (gDirectionalLights[0].enable)
    {
        shadowFactor = CalculateShadow(input.shadowCoord);
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
    }
    else
    {
        finalColor = baseColor * gMaterial.color.rgb;
    }

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
float3 Palette(float t)
{
    float3 a = float3(0.5f, 0.5f, 0.5f);
    float3 b = float3(0.5f, 0.5f, 0.5f);
    float3 c = float3(1.0f, 1.0f, 1.0f);
    float3 d = float3(0.268f, 0.416f, 0.557f);
    
    return a + b * cos(6.28318f * (c * t + d));
}
float stepping(float t)
{
    if (t < 0.0)
        return -1.0 + pow(1.0 + t, 2.0);
    else
        return 1.0 - pow(1.0 - t, 2.0);
}
float lineSegment(float2 A, float2 B, float2 C, float thickness)
{
    float2 AB = B - A;
    float2 AC = C - A;

    float t = dot(AC, AB) / dot(AB, AB);
    t = saturate(t); // clamp(0.0, 1.0)

    float2 Q = A + t * AB;
    float dist = length(Q - C);

    // 距離に基づくスムーズなライン表示
    return smoothstep(-0.01, -dist, -thickness) + smoothstep(-0.02, dist, thickness);
}
float rand(float2 n)
{
    return frac(sin(dot(n, float2(12.9898, 12.1414))) * 83758.5453);
}
float noise(float2 n)
{
    float2 d = float2(0.0, 1.0);
    float2 b = floor(n);
    float2 f = frac(n);
    return lerp(
        lerp(rand(b), rand(b + d.yx), f.x),
        lerp(rand(b + d.xy), rand(b + d.yy), f.x),
        f.y
    );
}
float fire(float2 n)
{
    return noise(n) + noise(n * 2.1) * 0.6 + noise(n * 5.4) * 0.42;
}
float3 ramp(float t)
{
    return (t <= 0.5)
        ? float3(1.0 - t * 1.4, 0.2, 1.05) / t
        : float3(0.3 * (1.0 - t) * 2.0, 0.2, 1.05) / t;
}
float3 getLine(float3 col, float2 fragCoord, float2x2 mtx, float shift, float time, float2 resolution)
{
    float2 uv = mul((fragCoord / resolution), mtx);

    uv.x += (uv.y < 0.5) ? 23.0 + time * 0.35 : -11.0 + time * 0.3;
    uv.y = abs(uv.y - shift);
    uv *= 5.0;

    float q = fire(uv - time * 0.013) / 2.0;
    float2 r = float2(
        fire(uv + q / 2.0 + time - uv.x - uv.y),
        fire(uv + q - time)
    );

    float grad = pow((r.y + r.y) * max(0.0, uv.y) + 0.1, 4.0);
    float3 color = ramp(grad);
    color /= (1.5 + max(0.0, color));

    if (color.b < 0.00000005)
        color = float3(0.0, 0.0, 0.0);

    return lerp(col, color, color.b);
}
float gridLine(float2 uv, float scale, float thickness)
{
    float2 grid = abs(frac(uv * scale - 0.5) - 0.5) / fwidth(uv * scale);
    float line1 = min(grid.x, grid.y);
    return smoothstep(0.0, thickness, line1);
}

float3 DrawArtWaveColor(PixelShaderInput input)
{
    float2 resolution = float2(720.0f, 720.0f);
    float2 screenUV = (input.texcoord * resolution - 0.5 * resolution);
    float2 uv = screenUV / resolution.y;

    float d = length(uv);
    float3 col = Palette(d); 

    float sinTime = d * 8.0f + gFrameData.gTime;
    d = sin(sinTime) / 8.0f;
    d = abs(d);
    d = 0.02f / d;

    col *= d;

    return col; 
}
float3 DrawArtQuadColor(PixelShaderInput input)
{
    float2 resolution = float2(720.0f, 720.0f); 
    float2 screenUV = input.texcoord * resolution;
    float2 u = (screenUV + screenUV - resolution) / resolution.y;

    float3 col = float3(0, 0, 0);
    float time = gFrameData.gTime;

    for (float i = 0.0; i < 20.0; i += 1.0)
    {
        float lenVal = length(u * u);
        float shapeDist = 0.004 / (abs(lenVal - i * 0.04) + 0.005);
        float3 colorMod = cos(float3(i, i + 1.0, i + 2.0)) + 1.0;

        float animVal = abs(abs(fmod(time, 2.0) - i * 0.1) - 1.0);
        float animSmooth = smoothstep(0.35, 0.4, animVal);

        col += shapeDist * colorMod * animSmooth;

        float angle = (time + i) * 0.03;
        float cosA = cos(angle);
        float sinA = sin(angle);
        float2x2 rotMat = float2x2(cosA, -sinA, sinA, cosA);
        u = mul(rotMat, u);
    }

    return col;
}
float3 DrawArtKikagakuColor(PixelShaderInput input)
{
    float2 resolution = gFrameData.iResolution;
    float2 fragCoord = input.texcoord * resolution;
    float2 uv = (fragCoord * 2.0 - resolution) / resolution.y;

    // normalize(uv) * length(uv)
    uv = normalize(uv) * length(uv);

    float3 col = float3(0, 0, 0);
    float time = gFrameData.gTime;

    [unroll]
    for (int i = 0; i < 12; i++)
    {
        float t = time + float(i) * 3.14159265 / 12.0 * (5.0 + stepping(sin(time * 3.0)));
        float2 p = float2(cos(t), sin(t));
        p *= cos(time + float(i) * 3.14159265 * cos(time / 8.0));

        float3 phase = float3(0.0, 1.0, -1.0) * (2.0 * 3.14159265 / 3.0);
        float3 colMod = cos(phase + 3.14159265 * (time / 2.0 + float(i) / 5.0)) * 0.5 + 0.5;

        float len = length(uv - p * 0.9);
        float intensity = 0.05 / max(len, 0.0001);

        col += intensity * colMod;
    }

    col = pow(col, float3(3.0, 3.0, 3.0));

    return col;
}
float3 DrawArtSoundColor(PixelShaderInput input)
{
    float2 resolution = gFrameData.iResolution;
    float2 fragCoord = input.texcoord * resolution;
    float2 uv = (fragCoord - 0.5 * resolution) / resolution.y;

    float3 color = float3(0.0, 0.0, 0.0);
    float time = gFrameData.gTime;

    [unroll]
    for (int i = 0; i < 20; i++)
    {
        float r = 0.7 - sin(time * 2.0 + float(i) * 0.8 * PI) * 0.2;
        float angle = time * 0.2 + float(i + 1) * 0.1 * PI;
        float2 dir = float2(cos(angle), sin(angle)) * r;

        float2 A = -dir * 0.5;
        float2 B = -dir * 0.3;

        float t = time * 0.5 + float(i) * 0.1 * TAU;

        float3 rgb = float3(
            sin(t) * 0.5 + 0.5,
            sin(t + PI / 2.0) * 0.5 + 0.5,
            sin(t + PI) * 0.5 + 0.5
        );

        color += lineSegment(A, B, uv, 0.001f) * rgb;
    }

    // Spiney-style highlight softening
    color = color * 0.4 + sqrt(color * color / (color * color + 1.0)) * 0.6;
    
    return color;
}

float3 DrawArtFragColor(PixelShaderInput input)
{
    float2 fragCoord = input.texcoord * gFrameData.iResolution;
    float2 uv = fragCoord / gFrameData.iResolution;
    float time = gFrameData.gTime;

    // 任意の変換行列（方向違いのライン）
    float2x2 m1 = float2x2(1.0, 0.0, 1.0, 1.0);
    float2x2 m2 = float2x2(1.0, 1.0, 1.0, 0.0);

    float3 col = float3(0.0, 0.0, 0.0);
    col = getLine(col, fragCoord, m1, 1.02, time, gFrameData.iResolution);
    col = getLine(col, fragCoord, m2, 1.02, time, gFrameData.iResolution);
    col = getLine(col, fragCoord, m1, -0.02, time, gFrameData.iResolution);
    col = getLine(col, fragCoord, m2, -0.02, time, gFrameData.iResolution);

    return col;
}

float3 DrawArtGridColor(PixelShaderInput input)
{
    float2 fragCoord = input.texcoord * gFrameData.iResolution;

    // UV原点を中央に（-0.5～+0.5）
    float2 uv = input.texcoord - 0.5;

    // ±10000範囲に変換（スプライトが10000x10000単位で扱う）
    uv *= 10000.0;

    // グリッド線の設定
    float scale = 1.0;
    float thickness = 1.5;

    // 通常のグリッド線
    float normalLine = gridLine(uv, scale, thickness);
    float gridMask = 1.0 - normalLine;

    // 10単位ごとの太線（強調ライン）
    float majorLineThickness = 1.5;
    float majorInterval = 10.0;

    float2 majorUV = uv / majorInterval;
    float majorLine = gridLine(majorUV, 1.0, majorLineThickness);
    float majorMask = 1.0 - majorLine;

    // 線の優先度（太線優先）
    float finalGridMask = max(gridMask, majorMask);

    // 背景色・グリッド色
    float3 bgColor = float3(0.05, 0.05, 0.05);
    float3 lineColor = float3(0.07, 0.07, 0.07);
    float3 majorLineColor = float3(0.20, 0.20, 0.20);

    // 線を重ねる
    float3 col = lerp(bgColor, lineColor, gridMask);
    col = lerp(col, majorLineColor, majorMask);

    // 原点軸の太さとカラー
    float axisThickness = 2.0;

    // Z軸 = 緑
    float zAxis = smoothstep(0.0, 1.0, abs(uv.x) / (fwidth(uv.x) * axisThickness));
    col = lerp(col, float3(0.1, 0.6, 0.1), 1.0 - zAxis);

    // X軸 = 赤
    float xAxis = smoothstep(0.0, 1.0, abs(uv.y) / (fwidth(uv.y) * axisThickness));
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

        float3 lightDir = normalize(-gDirectionalLights[i].direction);
        float ndotl = saturate(dot(normal, lightDir));
        float3 lightColor = gDirectionalLights[i].color.rgb * gDirectionalLights[i].color.a;
        float lightIntensity = gDirectionalLights[i].intensity;

        // diffuseとspecularをここで定義・初期化
        float3 diffuse = float3(0.0f, 0.0f, 0.0f);
        float3 specular = float3(0.0f, 0.0f, 0.0f);

        if (gMaterial.lightMode == LIGHT_HALFLAMBERT)
        {
            float halfLambert = pow(ndotl * 0.5f + 0.5f, 4.0f);
            // finalColorに足さず、diffuseに代入
            diffuse = gMaterial.color.rgb * baseColor * lightColor * halfLambert * lightIntensity;
        }
        else if (gMaterial.lightMode == LIGHT_PHONG_SPECULAR)
        {
            // Diffuse計算
            float halfLambert = pow(ndotl * 0.5f + 0.5f, 6.0f);
            diffuse = gMaterial.color.rgb * baseColor * lightColor * halfLambert * lightIntensity;

            // Specular計算
            if (ndotl > 0.0f)
            {
                float3 halfVec = normalize(lightDir + toEye);
                float spec = pow(saturate(dot(normal, halfVec)), gMaterial.shininess);
                specular = gMaterial.specularColor.rgb * lightColor * spec * lightIntensity;
            }
        }
        else if (gMaterial.lightMode == LIGHT_TOON)
        {
            float3 toonColor =
                ndotl > 0.7f ? baseColor :
                ndotl > 0.3f ? baseColor * 0.25f :
                               baseColor * 0.04f;
            diffuse = toonColor * lightColor * lightIntensity; // lightIntensity忘れずに
        }

        // ▼▼▼ ここで影を適用 ▼▼▼
        // 0番目のライトのみ影を落とす設定
        if (i == 0)
        {
            diffuse *= shadowFactor;
            specular *= shadowFactor;
        }

        // 最後にまとめて加算
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

        // 1. ライトの中心からピクセルへのベクトルを計算
        float3 vecToPixel = worldPos - gAreaLights[i].position;

        // 2. ライトのローカル軸（right, up）へピクセルを射影
        float3 rightDir = normalize(gAreaLights[i].right);
        float3 upDir = normalize(gAreaLights[i].up);
        float halfWidth = length(gAreaLights[i].right);
        float halfHeight = length(gAreaLights[i].up);

        float projRight = dot(vecToPixel, rightDir);
        float projUp = dot(vecToPixel, upDir);

        // 3. 射影した点を矩形の範囲内にクランプ（はみ出さないようにする）
        float clampedRight = clamp(projRight, -halfWidth, halfWidth);
        float clampedUp = clamp(projUp, -halfHeight, halfHeight);

        // 4. クランプした位置から「ピクセルに最も近いライト表面上の点」を再構築
        float3 closestPointOnLight = gAreaLights[i].position +
                                     rightDir * clampedRight +
                                     upDir * clampedUp;

        // 5. "最も近い点" を光源として、点光源と同様の計算を行う
        float3 lightVec = closestPointOnLight - worldPos;
        float distance = length(lightVec);
        float3 lightDir = normalize(lightVec); // これが実質的なライト方向

        // 6. 減衰の計算 
        float attenuation = gAreaLights[i].range > 0.001f
            ? pow(saturate(1.0f - distance / gAreaLights[i].range), gAreaLights[i].decay)
            : 1.0f;

        // 7. ディフューズ（拡散光）
        float ndotl = saturate(dot(normal, lightDir));
        float3 diffuse = gMaterial.color.rgb * baseColor * gAreaLights[i].color.rgb * ndotl * gAreaLights[i].intensity * attenuation;
        finalColor += diffuse;

        // 8. スペキュラ（鏡面反射）
        if (ndotl > 0.0f)
        {
            float3 halfVec = normalize(lightDir + toEye);
            float spec = pow(saturate(dot(normal, halfVec)), gMaterial.shininess);
            float3 specular = gMaterial.specularColor.rgb * gAreaLights[i].color.rgb * gAreaLights[i].intensity * spec * attenuation;
            finalColor += specular;
        }
    }

    return finalColor;
}

// 影の濃さを計算する関数
float CalculateShadow(float4 shadowCoord)
{
    // 1. 透視除算 (w除算)
    // 平行光源(正射影)ならw=1なので実質不要ですが、汎用性のために行います
    float3 projCoords = shadowCoord.xyz / shadowCoord.w;

    // 2. クリップ空間(-1~1)からUV空間(0~1)へ変換
    projCoords.x = projCoords.x * 0.5f + 0.5f;
    projCoords.y = -projCoords.y * 0.5f + 0.5f; // Y反転に注意

    // 範囲外判定 (シャドウマップの外なら影にしない)
    if (projCoords.z > 1.0f || projCoords.z < 0.0f ||
        projCoords.x > 1.0f || projCoords.x < 0.0f ||
        projCoords.y > 1.0f || projCoords.y < 0.0f)
    {
        return 1.0f; // 影なし
    }

    // 3. 深度比較 (PCFなしの単純比較の場合)
    // float currentDepth = projCoords.z;
    // float shadowMapDepth = gShadowMap.Sample(gSampler, projCoords.xy).r;
    // if (currentDepth - 0.005f > shadowMapDepth) return 0.5f; // 影あり(0.5倍)

    // 3. 深度比較 (PCFあり・比較サンプラー使用・推奨)
    // SampleCmpLevelZero は、(マップ値 < 比較値) なら 0、勝てば 1 を返します
    // つまり、(マップの深度 < 現在の深度) なら「奥にある＝影」なので 0 が返る
    float bias = 0.005f; // シャドウアクネ対策のバイアス
    float shadowFactor = gShadowMap.SampleCmpLevelZero(
        gShadowSampler,
        projCoords.xy,
        projCoords.z - bias
    );

    return shadowFactor; // 1.0(日向) ～ 0.0(影)
}