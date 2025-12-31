#include "FullScreenQuad.hlsli"

Texture2D gTexture : register(t0);
// 深度テクスチャ
Texture2D gDepthTex : register(t1); 
SamplerState gSampler : register(s0);

cbuffer PostEffectSettings : register(b0)
{
    float pixelationSize;
    float3 _padding0;

    float2 screenResolution;
    float grayscaleColorAmount;
    float sepiaColorAmount;

    float tintMulColorAmount;
    float tintAddColorAmount;
    float tintScreenColorAmount;
    float _padding1;

    float3 tintColor;
    float totalTime;

    float vignetteAmount;
    float vignetteRadius;
    float vignetteSoftness;
    float padding_1;
    
    float2 vignetteEllipseScale;
    float2 padding_2;

    float noiseAmount;
    float noiseSpeed;
    float noiseScale;
    float _padding2;

    float celShadingLevels;
    float chromaOffset;
    float waveAmplitude;
    float waveFrequency;

    int waveDirection;
    float waveSpeed;
    float fisheyeDistortion;
    float _paddingFisheye;

    float scanlineIntensity;
    float scanlineFrequency;
    int scanlineDirection;
    float _padding3;

    float3 scanlineColor;
    float scanlineScrollSpeed;

    float blockNoiseAmount;
    float blockNoiseSize;
    float blockNoiseSpeed;
    float _padding4;

    float rgbSplitOffset;
    float filmGrainIntensity;
    float _padding5;
    float _padding6;

    float glitchBlockHeight;
    float glitchAmount;
    float glitchNoiseIntensity;
    float _padding7;

    float heatDistortionStrength;
    float heatSpeed;
    float heatNoiseScale;
    float _padding8;

    float3 vignetteColor;
    float _padding9;

    float turbulentStrength;
    float turbulentFrequency;
    float turbulentSpeed;
    float _paddingTurbulence;

    int2 flag;
    float2 _paddingGlow2;
}

// 擬似乱数関数
float random(float2 uv)
{
    return frac(sin(dot(uv.xy, float2(12.9898, 78.233))) * 43758.5453);
}

float random(float2 uv, float time)
{
    float2 seed = uv * 1000.0 + time * 100.0;
    return frac(sin(dot(seed, float2(12.9898, 78.233))) * 43758.5453);
}

float noise(float2 uv, float time)
{
    // 複数スケールでノイズ生成
    float n1 = random(uv * 300.0 + time * 10.0);
    float n2 = random(uv * 600.0 - time * 15.0);
    float n3 = random(uv * 1200.0 + time * 20.0);
    // 合成して細かいノイズに
    return (n1 + n2 * 0.5 + n3 * 0.25) / 1.75;
}

// 2D hash: 小さい乱数を生成する
float hash(float2 p)
{
    return frac(sin(dot(p, float2(127.1, 311.7))) * 43758.5453);
}

// White Noise
float whiteNoise(float2 uv)
{
    return hash(uv);
}

// パーリンノイズ補完
float fade(float t)
{
    return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
}

// 擬似乱数生成
float grad(int hash, float2 p)
{
    return (hash & 15) < 8 ? p.x : p.y;
}

// パーリンノイズ生成
float perlinNoise(float2 uv)
{
    float2 p = floor(uv);
    float2 f = uv - p;

    int i = int(p.x) + int(p.y) * 57;
    int i1 = i + 1;
    int i2 = i + 57;
    int i3 = i2 + 1;

    float g1 = grad(i, f);
    float g2 = grad(i1, f - float2(1.0, 0.0));
    float g3 = grad(i2, f - float2(0.0, 1.0));
    float g4 = grad(i3, f - float2(1.0, 1.0));

    float u = fade(f.x);
    float v = fade(f.y);

    return lerp(v, lerp(u, g1, g2), lerp(u, g3, g4));
}


// 高自由度 FBM：オクターブ数、ラフネス、スケール、回転などに対応
float FBM(float2 p, int octaves, float gain, float lacunarity)
{
    float amplitude = 0.5;
    float frequency = 1.0;
    float sum = 0.0;

    for (int i = 0; i < octaves; i++)
    {
        sum += amplitude * perlinNoise(p * frequency);
        frequency *= lacunarity;
        amplitude *= gain;
    }

    return sum;
}

float4 main(VSOutput input) : SV_TARGET
{
    float2 uv = input.uv;
    
    float2 center = float2(0.5, 0.5);
    float2 coord = uv - center;
    float aspectRatio = screenResolution.x / screenResolution.y;
    
    if ((flag.x & PIXELATION) != 0)
    {
        float2 pixelSizeUV = pixelationSize / screenResolution;
        uv = floor(uv / pixelSizeUV) * pixelSizeUV;
    }
    if ((flag.x & SCREEN_WAVE) != 0)
    {
        float2 waveOffset = float2(0.0, 0.0);

        // 横方向（X座標に揺らぎ → 横波）
        if (waveDirection == 0 || waveDirection == 2) // 横 or 両方
        {
            float waveX = sin(uv.y * waveFrequency + totalTime * waveSpeed) * waveAmplitude;
            waveOffset.x += waveX;
        }
        
        float edgeFadeX = 1.0 - abs(uv.x - 0.5) * 2.0;
        float edgeFadeY = 1.0 - abs(uv.y - 0.5) * 2.0;
        
        float edgeFade = saturate(edgeFadeX * edgeFadeY);
        
        edgeFade = smoothstep(0.0, 0.2, edgeFade);

        // 縦方向（Y座標に揺らぎ → 縦波）
        if (waveDirection == 1 || waveDirection == 2) // 縦 or 両方
        {
            float waveY = sin(uv.x * waveFrequency + totalTime * waveSpeed) * waveAmplitude;
            waveOffset.y += waveY;
        }

        uv += waveOffset * edgeFade;
        uv = saturate(uv);
    }
    if ((flag.x & HEAT_HAZE) != 0)
    {
        float2 noiseUV = uv * heatNoiseScale + float2(totalTime * heatSpeed, totalTime * heatSpeed * 0.3);

        // FBMでX・Yそれぞれにノイズを生成
        float noiseX = FBM(noiseUV + float2(13.0, 7.0), 5, 0.5, 2.0);
        float noiseY = FBM(noiseUV + float2(21.0, 11.0), 5, 0.5, 2.0);

        // -0.5〜+0.5に調整して変位ベクトルに
        float2 offset = (float2(noiseX, noiseY) - 0.5) * heatDistortionStrength;


        // UV変形・サンプル
        uv += offset;
        uv = saturate(uv);
    }
    if ((flag.x & WATER_REFRACTION) != 0)
    {
           // UVを時間によるオフセットを含めて調整
        float2 noiseUV = uv * turbulentFrequency + float2(totalTime * turbulentSpeed, 0.0);

        // より軽量なノイズ関数（FBMの代わり）
        float displacement = sin(noiseUV.x * 10.0 + totalTime * 0.5) * 0.5 + sin(noiseUV.y * 10.0 + totalTime * 0.5) * 0.5;

        // 歪ませる方向に対する強度を適用
        float2 offset = float2(displacement, displacement) * turbulentStrength;

        // UVにオフセットを加えて歪ませる
        uv += offset;
      
    }
 
    float4 color = gTexture.Sample(gSampler, uv);
    

    // エフェクトの適用
    if ((flag.x & GRAYSCALE) != 0)
    {
        float gray = dot(color.rgb, float3(0.299, 0.587, 0.114));
        color.rgb = lerp(color.rgb, float3(gray, gray, gray), grayscaleColorAmount);
    }
    if ((flag.x & SEPIA) != 0)
    {
        float3 sepia = float3(
            dot(color.rgb, float3(0.393, 0.769, 0.189)),
            dot(color.rgb, float3(0.349, 0.686, 0.168)),
            dot(color.rgb, float3(0.272, 0.534, 0.131))
        );

        color.rgb = lerp(color.rgb, sepia, sepiaColorAmount);
    }
    if ((flag.x & COLOR_TINT) != 0)
    {
        // 掛け算×補間
        float3 mulTint = lerp(color.rgb, color.rgb * tintColor, tintMulColorAmount);

        // 加算合成
        float3 addTint = saturate(color.rgb + tintColor * tintAddColorAmount);

        // スクリーン合成
        float3 screenTint = 1.0 - (1.0 - color.rgb) * (1.0 - tintColor * tintScreenColorAmount);

        color.rgb = (mulTint + addTint + screenTint) / 3.0;
    }
  
    if ((flag.x & CEL_SHADING) != 0)
    {
       // 明度（輝度）の取得
        float luminance = dot(color.rgb, float3(0.299, 0.587, 0.114));

        // トーンの段階数（最低2）
        float levels = max(2.0, celShadingLevels);

        // 段階的に丸めた明度を求める
        float step = floor(luminance * levels) / (levels - 1.0);
    
        // 明度だけを段階化して、元の色にスケールを適用
        float3 toonColor = color.rgb * (step / max(luminance, 1e-5));

        // 最終カラー
        color.rgb = saturate(toonColor);
    }

    if ((flag.x & VIGNETTE) != 0)
    {
        float2 offset = uv - float2(0.5, 0.5);
        offset /= vignetteEllipseScale;

        // 距離の二乗を計算
        float distSqr = dot(offset, offset);

        // スムーズにフェードアウト（開始半径と柔らかさは同じ意味）
        float vignette = smoothstep(vignetteRadius, vignetteRadius + vignetteSoftness, distSqr);

        // 色を混ぜる
        color.rgb = lerp(color.rgb, vignetteColor, vignette * vignetteAmount);
    }
    if ((flag.x & SCREEN_NOISE) != 0)
    {
        float2 scaledUV = uv * screenResolution.xy * noiseScale;

        // 動きの方向と時間によるオフセット
        float2 timeOffset = float2(totalTime * noiseSpeed, totalTime * noiseSpeed * 1.7);

        // RGBそれぞれ少しずらしたノイズ
        float nR = sin(dot(scaledUV + timeOffset.xy, float2(12.9898, 78.233))) * 43758.5453;
        float nG = sin(dot(scaledUV + timeOffset.xy + 3.0, float2(12.9898, 78.233))) * 43758.5453;
        float nB = sin(dot(scaledUV + timeOffset.xy + 6.0, float2(12.9898, 78.233))) * 43758.5453;

        nR = frac(nR);
        nG = frac(nG);
        nB = frac(nB);

        // ノイズ量を適用
        float3 noiseColor = float3(nR, nG, nB);
        color.rgb += (noiseColor - 0.5) * noiseAmount;
    }
    if ((flag.x & CHROM_ABERRATION) != 0)
    {
        // UVをピクセル単位でオフセットするために画面解像度を使う
        float2 offset = chromaOffset / screenResolution;

        // R,G,Bそれぞれ微妙にずらす
        float red = gTexture.Sample(gSampler, uv + offset).r;
        float green = gTexture.Sample(gSampler, uv).g;
        float blue = gTexture.Sample(gSampler, uv - offset).b;

        color = float4(red, green, blue, 1.0);
    }
    if ((flag.x & FISHEYE) != 0)
    {
        float2 center = float2(0.5, 0.5);
        float2 offset = uv - center;
        float dist = length(offset);

        float distortion = fisheyeDistortion;

        float distDistorted = dist + distortion * dist * dist;

        float2 newUV = center + (offset / max(dist, 0.0001)) * distDistorted;

        // newUV が範囲外なら黒にする
        if (any(newUV < 0.0) || any(newUV > 1.0))
        {
            color = float4(0, 0, 0, 1);
        }
        else
        {
            newUV = saturate(newUV);
            color = gTexture.Sample(gSampler, newUV);
        }
    }

    if ((flag.x & SCANLINE) != 0)
    {
        float wave = 0.0;

        if (scanlineDirection == 0) // 横スキャン：Y方向に線（横線）
        {
            float input = uv.y * scanlineFrequency + totalTime * scanlineScrollSpeed;
            wave = sin(input * 6.28318);
        }
        else if (scanlineDirection == 1) // 縦スキャン：X方向に線（縦線）
        {
            float input = uv.x * scanlineFrequency + totalTime * scanlineScrollSpeed;
            wave = sin(input * 6.28318);
        }
        else if (scanlineDirection == 2) // 斜めスキャン（左上→右下）
        {
            float input = (uv.x + uv.y) * 0.7071 * scanlineFrequency + totalTime * scanlineScrollSpeed;
            wave = sin(input * 6.28318);
        }

        float mask = (wave + 1.0) * 0.5;
        float lerpFactor = mask * scanlineIntensity;
        color.rgb = lerp(color.rgb, scanlineColor.rgb, lerpFactor);
    }
    if ((flag.x & BLOCK_NOISE) != 0)
    {
        float2 blockSize = float2(blockNoiseSize, blockNoiseSize);
        float2 blockUV = floor(uv * screenResolution / blockSize);

        // より変化しやすく、乱数に効く形で時間を混ぜる
        float2 timeOffset = frac(totalTime * blockNoiseSpeed * float2(17.0, 23.0));

        float n = random(blockUV + timeOffset); // 変化するように

        float3 noiseColor = float3(n, n, n);
        color.rgb = lerp(color.rgb, noiseColor, blockNoiseAmount);
    }
    if ((flag.x & RGB_SPLIT) != 0)
    {
        // オフセット値をUV空間で指定
        float2 offset = float2(rgbSplitOffset, 0);

        // R,G,BそれぞれをX方向に少しずつずらす
        float r = gTexture.Sample(gSampler, saturate(uv - offset)).r;
        float g = gTexture.Sample(gSampler, uv).g;
        float b = gTexture.Sample(gSampler, saturate(uv + offset)).b;

        color = float4(r, g, b, 1.0);
    }
    if ((flag.x & FILM_GRAIN) != 0)
    {
        float intensity = filmGrainIntensity;

        float grain = noise(uv, totalTime);

        // 中心化（-0.5〜0.5）
        grain -= 0.5;

        // RGBそれぞれ少しずつずらして色むらを演出
        float grainR = grain * (0.9 + 0.2 * random(uv * 10.0 + float2(1.0, 0.0)));
        float grainG = grain * (0.9 + 0.2 * random(uv * 10.0 + float2(0.0, 1.0)));
        float grainB = grain * (0.9 + 0.2 * random(uv * 10.0 + float2(1.0, 1.0)));

        float3 grainColor = float3(grainR, grainG, grainB);

        color.rgb += grainColor * intensity;

        color.rgb = saturate(color.rgb);
    }
    if ((flag.x & GLITCH) != 0)
    {
        float blockHeight = glitchBlockHeight;
        float noiseIntensity = glitchNoiseIntensity;

        float blockIndex = floor(uv.y / blockHeight);
        float timePhase = totalTime * 0.5;

        float glitchOffsetX = (FBM(uv * 10.0 + float2(timePhase, 0.0), 4, 0.5, 2.0) - 0.5) * glitchAmount;
        float glitchOffsetY = (FBM(uv * 10.0 + float2(0.0, timePhase), 4, 0.5, 2.0) - 0.5) * glitchAmount * 0.3;

        float glitchSwitch = step(0.5, random(float2(blockIndex, floor(totalTime * 5.0))));

        float2 glitchUV = uv;
        glitchUV.x += glitchOffsetX * glitchSwitch;
        glitchUV.y += glitchOffsetY * glitchSwitch;
        
        float2 offsetR = float2(0.003 * sin(totalTime), 0.0);
        float2 offsetG = float2(-0.003 * cos(totalTime), 0.0);
        float2 offsetB = float2(0.003 * sin(uv.y * 50.0 + totalTime), 0.0);

        float r = gTexture.Sample(gSampler, glitchUV + offsetR).r;
        float g = gTexture.Sample(gSampler, glitchUV + offsetG).g;
        float b = gTexture.Sample(gSampler, glitchUV + offsetB).b;

        float3 glitchColor = float3(r, g, b);

        float noise = (whiteNoise(uv * screenResolution.xy + totalTime * 100.0) - 0.5) * noiseIntensity;
        glitchColor += noise;

        color.rgb = saturate(glitchColor);
    }
    return color;
}