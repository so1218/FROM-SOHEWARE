#include "FullScreenQuad.hlsli"

Texture2D gTexture : register(t0);
// 深度テクスチャ
Texture2D gDepthTex : register(t1); // 深度テクスチャ（t1）
SamplerState gSampler : register(s0);

cbuffer PostEffectSettings : register(b0)
{
    int mode;
    float brightnessValue;
    float posterizationLevels;
    float pixelationSize;
    
    float2 screenResolution;
    float grayscaleColorAmount;
    float sepiaColorAmount;
        
    float invertColorAmount;
    float tintMulColorAmount;
    float tintAddColorAmount;
    float tintScreenColorAmount;
    
    float3 tintColor;
    float totalTime;
    
    float contrastValue;
    float saturationValue;
    float hueShiftAmount;
    float vignetteAmount;
    
    float vignetteRadius;
    float vignetteSoftness;
    float2 vignetteEllipseScale;
    
    int channelSwapMode;
    float noiseAmount;
    float noiseSpeed;
    float noiseScale;
    
    float celShadingLevels;
    float normalOutlineThreshold;
    float normalOutlineThickness;
    float _paddingNormlOutline1;

    float3 normalOutlineColor;
    float _paddingNormlOutline2;
    
    float chromaOffset;
    float waveAmplitude;
    float waveFrequency;
    int waveDirection;
    
    float waveSpeed;
    float fisheyeDistortion;
    float2 _paddingFisheye;
   
    float flashFrequency;
    float flashIntensity;
    float scanlineIntensity;
    float scanlineFrequency;
    
    int scanlineDirection;
    float3 scanlineColor;
    
    float scanlineScrollSpeed;
    float3 _paddingscanline;
    
    float blockNoiseAmount;
    float blockNoiseSize;
    float blockNoiseSpeed;
    float solarizeThreshold;
    
    float multiPosterizeLevels;
    float rgbSplitOffset;
    float filmGrainIntensity;
    float _padding1;
    
    float glitchBlockHeight;
    float glitchAmount;
    float glitchNoiseIntensity;
    float edgeThreshold;
    
    float heatDistortionStrength;
    float heatSpeed;
    float heatNoiseScale;
    float _padding2;
    
    float3 vignetteColor;
    float _padding3;
    
    float3 shadowColor;
    float _paddingSplit;
    
    float3 highlightColor;
    float splitToneStrength;
    
    float turbulentStrength;
    float turbulentFrequency;
    float turbulentSpeed;
    float _paddingTurbulence;

    float roughEdgeThreshold;
    float roughEdgeRoughness;
    float roughEdgeNoiseScale;
    float roughEdgeSpeed;
    
    float3 roughEdgeColor;
    float _paddingRoughEdge;
    
    float spiralBaseAmplitude;
    float spiralFrequency;
    float spiralDistanceFalloff;
    float spiralNoiseAmount;
    
    float spiralNoiseSpeed;
    float spiralNoiseScale;
    float spiralRotationSpeed;
    float spiralSpeed;

    float radialWaveSpeed;
    float radialWaveAmplitude;
    float radialWaveFrequency;
    float _paddingRadialWave;
    
    float glowOutlineThreshold;
    float glowOutlineThickness;
    float glowOutlineIntensity;
    float _paddingGlow0;
    
    float3 glowOutlineColor;
    float _paddingGlow1;
    
    int2 flag;
    float2 _paddingGlow2;
    
    int fbmOctaves;
    float fbmGain;
    float fbmLacunarity;
    float fbmSharpness;
    
    float fbmNoiseIntensity;
    float3 fbmNoiseColor;
    
    float3 flareColor;
    float flareIntensity;

    float flareFalloff;
    float flareGhostDistance;
    float flareGhostIntensity;
    float flareStreakCount;

    float flareStreakSpeed;
    float flareStreakSharpness;
    float flareStreakIntensity;
    float paddingFlare;
    
    float2 ballRadiusValue;
    float2 ballPosition;
    
    float ballNoiseAmount;
    float3 ballColorAdjustment;
    
    float ballTimeSpeed;
    float dotBlinkSize;
    float dotBlinkSpeed;
    float _paddingdotBlink;

}


// RGB → HSV 変換
float3 RGBToHSV(float3 rgb)
{
    float4 K = float4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    float4 p = lerp(float4(rgb.bg, K.z, K.w), float4(rgb.gb, K.x, K.y), step(rgb.b, rgb.g));
    float4 q = lerp(float4(p.xyw, rgb.r), float4(rgb.r, p.yzx), step(p.x, rgb.r));

    float d = q.x - min(q.w, q.y);
    float e = 1e-10;

    return float3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

// HSV → RGB 変換
float3 HSVToRGB(float3 hsv)
{
    float4 K = float4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    float3 p = abs(frac(hsv.x + K.yzw) * 6.0 - K.www);
    return hsv.z * lerp(K.xxx, saturate(p - K.xxx), hsv.y);
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

float SimpleNoise(float2 uv)
{
    return frac(sin(dot(uv, float2(12.9898, 78.233))) * 43758.5453);
}

// 線形補間した滑らかなノイズ
float smoothNoise(float2 uv)
{
    float2 i = floor(uv);
    float2 f = frac(uv);

    // 4つの周辺点のランダム値
    float a = random(i);
    float b = random(i + float2(1, 0));
    float c = random(i + float2(0, 1));
    float d = random(i + float2(1, 1));

    // 補間関数（滑らかにする）
    float2 u = f * f * (3.0 - 2.0 * f);

    return lerp(lerp(a, b, u.x), lerp(c, d, u.x), u.y);
}


// ノイズ関数（擬似FBM）
float FBM(float2 uv)
{
    float val = 0.0;
    float amplitude = 0.5; // 各オクターブの振幅
    float frequency = 1.0; // 各オクターブの周波数

    for (int i = 0; i < 4; ++i)
    {
        // ここを SimpleNoise に変更
        val += SimpleNoise(uv * frequency) * amplitude;

        amplitude *= 0.5;
        frequency *= 2.0;
    }
    return val;
}

float GetEdge(float2 uv)
{
    float2 texel = 1.0 / screenResolution;
    float lumTL = dot(gTexture.Sample(gSampler, uv + texel * float2(-1, -1)).rgb, float3(0.299, 0.587, 0.114));
    float lumTR = dot(gTexture.Sample(gSampler, uv + texel * float2(1, -1)).rgb, float3(0.299, 0.587, 0.114));
    float lumBL = dot(gTexture.Sample(gSampler, uv + texel * float2(-1, 1)).rgb, float3(0.299, 0.587, 0.114));
    float lumBR = dot(gTexture.Sample(gSampler, uv + texel * float2(1, 1)).rgb, float3(0.299, 0.587, 0.114));
    float edge = abs(lumTL - lumBR) + abs(lumTR - lumBL);
    return saturate(edge);
}

float HandDrawnNoise(float2 uv)
{
    float n = sin(uv.x * 40.0 + sin(uv.y * 40.0 + totalTime * 2.0));
    return n * 0.5 + 0.5;
}

float Noise(float2 uv)
{
    // ここにパーリンノイズやSimplexノイズ、あるいはより複雑なフラクタルノイズの実装を入れる
    // 例として、元のSimpleNoiseを少し改変したもの
    return frac(sin(dot(uv, float2(12.9898, 78.233) * 0.1) + totalTime * 0.1) * 43758.5453);
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

// Simplexノイズ関数（簡易版）
float simplexNoise(float2 uv)
{
    // 3D空間に埋め込むことで擬似3Dノイズを2Dに投影
    float3 p = float3(uv, 0.0);
    p = p - floor(p);
    float4 grad = float4(1.0, 1.0, -1.0, -1.0);

    return grad.x * p.x + grad.y * p.y;
}

// 近傍のセルの生成
float worleyNoise(float2 uv)
{
    float2 p = floor(uv);
    float2 f = uv - p;
    
    float minDist = 1.0; // 最短距離
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float2 neighbor = float2(p.x + x, p.y + y);
            float2 diff = uv - neighbor;
            minDist = min(minDist, length(diff));
        }
    }
    return minDist;
}


// グラディエントノイズ：値を滑らかに補間
float FBMnoise(float2 p)
{
    float2 i = floor(p);
    float2 f = frac(p);
    float2 u = f * f * (3.0 - 2.0 * f);

    float a = hash(i + float2(0.0, 0.0));
    float b = hash(i + float2(1.0, 0.0));
    float c = hash(i + float2(0.0, 1.0));
    float d = hash(i + float2(1.0, 1.0));

    return lerp(lerp(a, b, u.x), lerp(c, d, u.x), u.y);
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
    float4 color = gTexture.Sample(gSampler, uv);
    
    float2 center = float2(0.5, 0.5);
    float2 coord = uv - center;
    float aspectRatio = screenResolution.x / screenResolution.y;

    // エフェクトの適用
    if ((flag.x & GRAYSCALE) != 0)
    {
        float gray = dot(color.rgb, float3(0.299, 0.587, 0.114));
        color.rgb = lerp(color.rgb, float3(gray, gray, gray), grayscaleColorAmount);
    }
    if ((flag.x & INVERT_COLOR) != 0)
    {
        float3 invertedColor = 1.0 - color.rgb;
        color.rgb = lerp(color.rgb, invertedColor, invertColorAmount);
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
    if ((flag.x & BRIGHTNESS) != 0)
    {
        color.rgb += brightnessValue;
    }
    if ((flag.x & POSTERIZATION) != 0)
    {
        float levels = max(2.0, posterizationLevels);
        color.rgb = floor(color.rgb * levels) / (levels - 1.0);
    }
    if ((flag.x & PIXELATION) != 0)
    {
        float2 pixelSizeUV = pixelationSize / screenResolution;
        uv = floor(uv / pixelSizeUV) * pixelSizeUV;
        
        color = gTexture.Sample(gSampler, uv);
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
    if ((flag.x & CONTRAST) != 0)
    {
        color.rgb = (color.rgb - 0.5) * contrastValue + 0.5;
    }
    if ((flag.x & SATURATION) != 0)
    {
        float gray = dot(color.rgb, float3(0.299, 0.587, 0.114));
        color.rgb = lerp(gray.xxx, color.rgb, saturationValue);
    }
    if ((flag.x & HUE_SHIFT) != 0)
    {
        float3 hsv = RGBToHSV(color.rgb);
        hsv.x = frac(hsv.x + hueShiftAmount);
        color.rgb = HSVToRGB(hsv);
    }
    if ((flag.x & CHANNEL_SWAP) != 0)
    {
        switch (channelSwapMode)
        {
            case 0:
                color.rgb = color.bgr;
                break;
            case 1:
                color.rgb = color.grb;
                break;
            case 2:
                color.rgb = color.gbr;
                break;
            case 3:
                color.rgb = color.brg;
                break;
            case 4:
                color.rgb = color.rbg;
                break;
        }
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
    if ((flag.x & NORMAL_OUTLINE) != 0)
    {
        float2 texel = normalOutlineThickness / screenResolution;

        float3 centerColor = gTexture.Sample(gSampler, uv).rgb;

        float maxDiff = 0.0;
        [unroll]
        for (int y = -1; y <= 1; y++)
        {
            [unroll]
            for (int x = -1; x <= 1; x++)
            {
                if (x == 0 && y == 0)
                    continue;

                float2 offset = float2(x, y) * texel;
                float2 sampleUV = saturate(uv + offset);
                float3 sampleColor = gTexture.Sample(gSampler, sampleUV).rgb;

                // 色差を計算
                float diff = length(centerColor - sampleColor);

                maxDiff = max(maxDiff, diff);
            }
        }

        // エッジ検出の閾値調整
        float edgeFactor = smoothstep(normalOutlineThreshold * 0.5, normalOutlineThreshold + 0.1, maxDiff);

        // 指定色でアウトライン。
        color.rgb = lerp(color.rgb, normalOutlineColor, edgeFactor);
    }
    if ((flag.x & BRIGHT_EXTRACT) != 0)
    {
        // 高輝度成分だけ抽出
        float luminance = dot(color.rgb, float3(0.299, 0.587, 0.114));
        float threshold = 0.8;
        color.rgb = (luminance > threshold) ? color.rgb : float3(0, 0, 0);
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
    if ((flag.x & SCREEN_WAVE) != 0)
    {
        float2 waveOffset = float2(0.0, 0.0);

        // 横方向（X座標に揺らぎ → 横波）
        if (waveDirection == 0 || waveDirection == 2) // 横 or 両方
        {
            float waveX = sin(uv.y * waveFrequency + totalTime * waveSpeed) * waveAmplitude;
            waveOffset.x += waveX;
        }

    // 縦方向（Y座標に揺らぎ → 縦波）
        if (waveDirection == 1 || waveDirection == 2) // 縦 or 両方
        {
            float waveY = sin(uv.x * waveFrequency + totalTime * waveSpeed) * waveAmplitude;
            waveOffset.y += waveY;
        }

        uv += waveOffset;
        uv = saturate(uv);

        color = gTexture.Sample(gSampler, uv);
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
    if ((flag.x & FLASH) != 0)
    {
        // 0〜1を周期的に変化させる
        float flash = (sin(totalTime * 6.28318 * flashFrequency) + 1.0) * 0.5;

        // 明るさを増加（色が飛びすぎないようにsaturateでクランプ）
        color.rgb = saturate(color.rgb + flash * flashIntensity);
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
    if ((flag.x & SOLARIZE) != 0)
    {
        float3 threshold = float3(solarizeThreshold, solarizeThreshold, solarizeThreshold); // 明るさの閾値

        // 条件付きで各色チャネルを反転
        color.r = (color.r > threshold.r) ? 1.0 - color.r : color.r;
        color.g = (color.g > threshold.g) ? 1.0 - color.g : color.g;
        color.b = (color.b > threshold.b) ? 1.0 - color.b : color.b;
    }
    if ((flag.x & MULTI_POSTERIZE) != 0)
    {
        float luminance = dot(color.rgb, float3(0.299, 0.587, 0.114));
        float levels = max(2.0, multiPosterizeLevels);
        float poster = floor(luminance * levels) / (levels - 1.0);
        color.rgb = float3(poster, poster, poster); // グレースケール版

        // もし元の色で色相を残したい場合
        // color.rgb *= poster;
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
    if ((flag.x & INVERT_BY_Y) != 0)
    {
        // uv.yは0〜1の縦位置（0が上、1が下）
        float invertAmount = uv.y;

        // 色反転した色
        float3 invertedColor = 1.0 - color.rgb;

        // 元の色と反転色を縦位置によって線形補間
        color.rgb = lerp(color.rgb, invertedColor, invertAmount);
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
    if ((flag.x & EDGE_DETECTION) != 0)
    {
        float2 texelSize = 1.0 / screenResolution.xy;

        float lumTL = dot(gTexture.Sample(gSampler, uv + texelSize * float2(-1, -1)).rgb, float3(0.299, 0.587, 0.114));
        float lumTC = dot(gTexture.Sample(gSampler, uv + texelSize * float2(0, -1)).rgb, float3(0.299, 0.587, 0.114));
        float lumTR = dot(gTexture.Sample(gSampler, uv + texelSize * float2(1, -1)).rgb, float3(0.299, 0.587, 0.114));
        float lumCL = dot(gTexture.Sample(gSampler, uv + texelSize * float2(-1, 0)).rgb, float3(0.299, 0.587, 0.114));
        float lumCR = dot(gTexture.Sample(gSampler, uv + texelSize * float2(1, 0)).rgb, float3(0.299, 0.587, 0.114));
        float lumBL = dot(gTexture.Sample(gSampler, uv + texelSize * float2(-1, 1)).rgb, float3(0.299, 0.587, 0.114));
        float lumBC = dot(gTexture.Sample(gSampler, uv + texelSize * float2(0, 1)).rgb, float3(0.299, 0.587, 0.114));
        float lumBR = dot(gTexture.Sample(gSampler, uv + texelSize * float2(1, 1)).rgb, float3(0.299, 0.587, 0.114));

        float gx = -lumTL - 2.0 * lumCL - lumBL + lumTR + 2.0 * lumCR + lumBR;
        float gy = -lumTL - 2.0 * lumTC - lumTR + lumBL + 2.0 * lumBC + lumBR;

        float edgeStrength = length(float2(gx, gy));

        float edge = smoothstep(edgeThreshold, edgeThreshold + 0.1, edgeStrength);
        float3 edgeColor = float3(0.0, 0.0, 0.0); // 白い線で描く
        color.rgb = lerp(color.rgb, edgeColor, edge); // エッジ部分だけ白く混ぜる
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
        color = gTexture.Sample(gSampler, uv);
    }
    if ((flag.x & SPLIT_TONING) != 0)
    {
        float luminance = dot(color.rgb, float3(0.299, 0.587, 0.114)); // 明度計算

        // シャドウ〜ハイライト間の色を線形補間
        float3 toneColor = lerp(shadowColor, highlightColor, luminance);

        // 元の色と補正色をブレンド
        color.rgb = lerp(color.rgb, toneColor, splitToneStrength);
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

    // 歪んだUVで再サンプリング
        color = gTexture.Sample(gSampler, uv);
    }
    if ((flag.y & ROUGH_EDGE) != 0)
    {
        float2 timeOffset = float2(totalTime * roughEdgeSpeed, totalTime * roughEdgeSpeed);

    // ノイズ生成を最適化（FBMを減らす）
        float2 baseNoiseUV = uv * roughEdgeNoiseScale + timeOffset;
        float baseNoise = FBM(baseNoiseUV); // 一度だけFBMを計算

    // ノイズのオフセット計算を簡素化
        float2 offsetUV = uv + (baseNoise - 0.5) * roughEdgeRoughness * 0.01;
        float edge = GetEdge(offsetUV);

    // ノイズを一度だけ計算し、複数回利用
        float secondaryNoise = FBM(uv * (roughEdgeNoiseScale * 0.5) + timeOffset); // 再利用
        edge += (secondaryNoise - 0.5) * roughEdgeRoughness * 0.5;

    // しきい値の調整を改善
        float noisyThreshold = roughEdgeThreshold + (FBM(uv * 2.0 + timeOffset) - 0.5) * 0.2;
        float mask = smoothstep(noisyThreshold - 0.1, noisyThreshold + 0.1, edge);

    // 最終的なマスクの調整
        mask *= (FBM(uv * 10.0 + timeOffset) * 0.5 + 0.5);

    // エッジカラーの適用：ユーザーが指定した色を使う
        float3 edgeColor = float3(roughEdgeColor.x, roughEdgeColor.y, roughEdgeColor.z) * 2.0;

    // 最終的な色のブレンド
        color.rgb = lerp(color.rgb, edgeColor, mask * abs(FBM(uv * 30.0 + timeOffset)));
    }
    if ((flag.y & SPIRAL_WARP) != 0)
    {
    // UV座標の中心を基準にして、中心との相対的な座標を求める
        float2 coord = uv - center;

    // 半径rと角度を計算
        float r = length(coord); // UV座標の中心からの距離
        float angle = atan2(coord.y, coord.x); // atan2で角度を取得

       // totalTime に spiralSpeed を掛けて回転速度調整
        float timeWithSpeed = totalTime * spiralSpeed;

        float baseRotation = spiralRotationSpeed * timeWithSpeed / (r + 0.01);

    // ノイズを用いた角度の変動
        float n = FBM(coord * spiralNoiseScale + totalTime * 1000);
        float noiseAngleOffset = (n - 0.5) * 2.0 * spiralNoiseAmount;

    // 総合的な回転角度
        angle += baseRotation * spiralBaseAmplitude + noiseAngleOffset;

    // 渦巻き後の座標（cos, sinを使って回転）
        float2 warpedCoord = float2(cos(angle), sin(angle)) * r;

    // 最終的なUV座標
        uv = warpedCoord + center; // 元の中心に戻す

    // 変形したUVで色をサンプリング
        color = gTexture.Sample(gSampler, uv);
    }
    if ((flag.y & RADIAL_WAVE) != 0)
    {
        float2 center = float2(0.5, 0.5); // 画面中心（UV空間）
        float2 coord = uv - center;

        float r = length(coord);
        float angle = atan2(coord.y, coord.x);

        // 円周波動の計算
        float wave = sin(r * radialWaveFrequency - totalTime * radialWaveSpeed) * radialWaveAmplitude;

        // 半径に波を加算してゆらぎを作る
        float2 warpedCoord = float2(cos(angle), sin(angle)) * (r + wave);

        // 元のUV空間に戻す
        uv = warpedCoord + center;
        
        uv = saturate(uv);
        color = gTexture.Sample(gSampler, uv);
   
    }
    if ((flag.y & GLOW_OUTLINE) != 0)
    {
        // 深度の差分を取ることでエッジを強調
        float depthCenter = gDepthTex.Sample(gSampler, uv).r; // 中心の深度
        float depthLeft = gDepthTex.Sample(gSampler, uv + float2(-1.0, 0.0) / screenResolution).r; // 左側の深度
        float depthRight = gDepthTex.Sample(gSampler, uv + float2(1.0, 0.0) / screenResolution).r; // 右側の深度
        float depthUp = gDepthTex.Sample(gSampler, uv + float2(0.0, -1.0) / screenResolution).r; // 上側の深度
        float depthDown = gDepthTex.Sample(gSampler, uv + float2(0.0, 1.0) / screenResolution).r; // 下側の深度

        // 中心の深度と周囲の深度差を計算
        float edgeDetection = abs(depthCenter - depthLeft) + abs(depthCenter - depthRight) +
                              abs(depthCenter - depthUp) + abs(depthCenter - depthDown);

        // エッジ検出によってアウトラインを強調
        float outlineThreshold = 0.05; // アウトラインを描画するための深度差の閾値
        if (edgeDetection > glowOutlineThreshold)
        {
            // エッジ部分は黒でアウトラインを描画
            color.rgb = glowOutlineColor.rgb; // 黒色でアウトライン
            color.a = 1.0; // 不透明にする
        }
    }
    if ((flag.y & FBM_NOISE) != 0)
    {
        float n = FBM(coord * spiralNoiseScale + totalTime * spiralNoiseSpeed, fbmOctaves, fbmGain, fbmLacunarity); // 5オクターブ、gain=0.5, lacunarity=2.0
        float noiseAngleOffset = (n - 0.5) * 2.0 * spiralNoiseAmount;
        float mask = smoothstep(0.5 - fbmSharpness, 0.5 + fbmSharpness, n);
        // 色 or グロー強度として使用
        float3 glow = fbmNoiseColor * mask * fbmNoiseIntensity;

        // 合成（発光風に加算）
        color.rgb = saturate(color.rgb + glow);
    }
    if ((flag.y & FLARE) != 0)
    {
        float2 flareCenter = float2(0.5, 0.5); // フレアの中心（画面中央）
        float2 delta = uv - flareCenter;
        float dist = length(delta);

        // フレア強度を距離に応じて減衰（0.0 に近いと中心）
        float attenuation = pow(1.0 - saturate(dist), flareFalloff);

        // 放射状のグロー
        float radial = smoothstep(0.0, 1.0, attenuation);

        // ゴースト（レンズ内部反射）
        float2 ghostUV = flareCenter - delta * flareGhostDistance;
        float4 ghostColor = gTexture.Sample(gSampler, saturate(ghostUV));
        ghostColor.rgb *= flareGhostIntensity;

        // 色付きの放射線
        float angle = atan2(delta.y, delta.x);
        float flareStreak = abs(sin(angle * flareStreakCount + totalTime * flareStreakSpeed));
        flareStreak = pow(flareStreak, flareStreakSharpness);

        float3 finalFlare = flareColor * radial * flareIntensity;
        finalFlare += ghostColor.rgb * flareColor;
        finalFlare += flareStreak * flareStreakIntensity;

        // 加算合成
        color.rgb += finalFlare;
    }
    if ((flag.y & BALL_EFFECT) != 0)
    {
        float2 ballCenter = ballPosition;
        float2 radius = ballRadiusValue;
        float2 dist = uv - ballCenter;

        // 楕円の長さを計算する
        float len = length(dist / radius);

        if (len < 1.0)
        {
            // ノイズ座標（球体範囲内だけ）
            float2 noiseUV = uv * 10.0 + float2(totalTime * ballTimeSpeed, 0.0);

            // ノイズ強度
            float fbmValue = FBM(noiseUV, 4, 0.5, 2.0); // 0〜1
            float noiseDisplace = (fbmValue - 0.5) * ballNoiseAmount;

            // X方向にノイズで歪ませる
            float2 scrollUV = uv;
            scrollUV.x += noiseDisplace * (1.0 - len); // 中心ほど強い

            // テクスチャ再取得
            float4 noisyColor = gTexture.Sample(gSampler, scrollUV);

            // 発光的な追加（色を colorAdjustment で制御）
            float3 glowColor = ballColorAdjustment.rgb * (1.0 - len); // 発光色を colorAdjustment.rgb で調整
            noisyColor.rgb += glowColor;

            // RGBAを操作
            // 透明度の変更:中心ほど不透明、外側ほど透明
            noisyColor.a = lerp(1.0, 0.0, len); // 透明度の補間（球の外に向かって透明度が下がる）

            // 球体領域だけ合成
            color.rgb = lerp(color.rgb, noisyColor.rgb, 1.0 - len);
            color.a = lerp(color.a, noisyColor.a, 1.0 - len); // アルファも合成
        }
    }
    if ((flag.y & DOT_BLINK) != 0)
    {
        // ピクセル位置をスクリーン解像度に基づいてグリッド化
        float2 pixelPos = uv * screenResolution;
        float2 gridPos = floor(pixelPos / dotBlinkSize); // グリッド化

        // 時間による点滅の周期を計算
        float blinkPeriod = totalTime * dotBlinkSpeed; // 点滅の速さ
        float pattern = frac(blinkPeriod + gridPos.x + gridPos.y); // グリッド位置ごとにオフセット

        // 点滅の状態
        float blink = step(0.5, pattern); // 0〜1の周期で点滅

        // 点滅効果
        color.rgb *= blink;
    }
    if ((flag.y & OUTLINE) != 0)
    {
        // 深度の差分を取ることでエッジを強調
        float depthCenter = gDepthTex.Sample(gSampler, uv).r; // 中心の深度
        float depthLeft = gDepthTex.Sample(gSampler, uv + float2(-1.0, 0.0) / screenResolution).r; // 左側の深度
        float depthRight = gDepthTex.Sample(gSampler, uv + float2(1.0, 0.0) / screenResolution).r; // 右側の深度
        float depthUp = gDepthTex.Sample(gSampler, uv + float2(0.0, -1.0) / screenResolution).r; // 上側の深度
        float depthDown = gDepthTex.Sample(gSampler, uv + float2(0.0, 1.0) / screenResolution).r; // 下側の深度

        // 中心の深度と周囲の深度差を計算
        float edgeDetection = abs(depthCenter - depthLeft) + abs(depthCenter - depthRight) +
                              abs(depthCenter - depthUp) + abs(depthCenter - depthDown);

        // エッジ検出によってアウトラインを強調
        float outlineThreshold = 0.05; // アウトラインを描画するための深度差の閾値
        if (edgeDetection > outlineThreshold)
        {
            // エッジ部分は黒でアウトラインを描画
            color.rgb = float3(0.0, 0.0, 0.0); // 黒色でアウトライン
            color.a = 1.0; // 不透明にする
        }
    }
     
    return color;
}