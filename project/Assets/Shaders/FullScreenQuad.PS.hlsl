#include "FullScreenQuad.hlsli"
#include "ShaderConstants.hlsli"

Texture2D gTexture : register(t0);
Texture2D gDissolveTexture : register(t1);
Texture2D gLutTexture : register(t2);
SamplerState gSampler : register(s0);
SamplerState gClampSampler : register(s1);

ConstantBuffer<PostEffectData> gData : register(b0);

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

// 2D hash:小さい乱数を生成する
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

// 高自由度FBM
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

// =======================================================
//  UV変形系
// =======================================================

// ピクセル化
float2 ApplyPixelation(float2 uv)
{
    float2 pixelSizeUV = gData.pixelationSize / gData.screenResolution;
    return floor(uv / pixelSizeUV) * pixelSizeUV;
}

// 画面波（横・縦・両方） 
float2 ApplyScreenWave(float2 uv)
{
    float2 waveOffset = float2(0.0, 0.0);
    // 横波
    if (gData.waveDirection == 0 || gData.waveDirection == 2)
    {
        waveOffset.x += sin(uv.y * gData.waveFrequency + gData.totalTime * gData.waveSpeed) * gData.waveAmplitude;
    }
    // 縦波
    if (gData.waveDirection == 1 || gData.waveDirection == 2)
    {
        waveOffset.y += sin(uv.x * gData.waveFrequency + gData.totalTime * gData.waveSpeed) * gData.waveAmplitude;
    }

    // 画面端のフェード処理（端で歪まないように）
    float edgeFade = saturate((1.0 - abs(uv.x - 0.5) * 2.0) * (1.0 - abs(uv.y - 0.5) * 2.0));
    edgeFade = smoothstep(0.0, 0.2, edgeFade);

    return saturate(uv + waveOffset * edgeFade);
}

// 陽炎
float2 ApplyHeatHaze(float2 uv)
{
    float2 noiseUV = uv * gData.heatNoiseScale + float2(gData.totalTime * gData.heatSpeed, gData.totalTime * gData.heatSpeed * 0.3);
    float noiseX = FBM(noiseUV + float2(13.0, 7.0), 5, 0.5, 2.0);
    float noiseY = FBM(noiseUV + float2(21.0, 11.0), 5, 0.5, 2.0);
    float2 offset = (float2(noiseX, noiseY) - 0.5) * gData.heatDistortionStrength;
    return saturate(uv + offset);
}

// 水面屈折
float2 ApplyWaterRefraction(float2 uv)
{
    float2 noiseUV = uv * gData.turbulentFrequency + float2(gData.totalTime * gData.turbulentSpeed, 0.0);
    float displacement = sin(noiseUV.x * 10.0 + gData.totalTime * 0.5) * 0.5 + sin(noiseUV.y * 10.0 + gData.totalTime * 0.5) * 0.5;
    return uv + float2(displacement, displacement) * gData.turbulentStrength;
}

// 魚眼レンズ
float2 ApplyFisheye(float2 uv)
{
    float2 center = float2(0.5, 0.5);
    float2 offset = uv - center;
    float dist = length(offset);
    float distDistorted = dist + gData.fisheyeDistortion * dist * dist;
    float2 newUV = center + (offset / max(dist, 0.0001)) * distDistorted;

    return newUV;
}

// =======================================================
//  カラー系
// =======================================================

// グレースケール
float3 ApplyGrayscale(float3 color)
{
    float gray = dot(color, float3(0.299, 0.587, 0.114));
    return lerp(color, float3(gray, gray, gray), gData.grayscaleColorAmount);
}

// セピア
float3 ApplySepia(float3 color)
{
    float3 sepia = float3(
        dot(color, float3(0.393, 0.769, 0.189)),
        dot(color, float3(0.349, 0.686, 0.168)),
        dot(color, float3(0.272, 0.534, 0.131))
    );
    return lerp(color, sepia, gData.sepiaColorAmount);
}

// 色かぶり
float3 ApplyColorTint(float3 color)
{
    float3 mulTint = lerp(color, color * gData.tintColor, gData.tintMulColorAmount);
    float3 addTint = saturate(color + gData.tintColor * gData.tintAddColorAmount);
    float3 screenTint = 1.0 - (1.0 - color) * (1.0 - gData.tintColor * gData.tintScreenColorAmount);
    return (mulTint + addTint + screenTint) / 3.0;
}

// ビネット
float3 ApplyVignette(float3 color, float2 uv)
{
    // 中心座標
    float2 center = uv - 0.5;
    
    // 距離の計算
    float dist = length(center / gData.vignetteEllipseScale);

    // 距離に基づいたリニアな進行度を計算
    // ゼロ除算対策
    float fadeLength = max(gData.vignetteSoftness, 0.0001);
    
    float darkness = (dist - gData.vignetteRadius) / fadeLength;
    
    // 0.0～1.0の範囲に切り取る
    darkness = saturate(darkness);

    // 滑らかにする
    darkness = smoothstep(0.0, 1.0, darkness);

    // 強度とディザリング
    darkness *= gData.vignetteAmount;

    float dither = (random(uv) - 0.5) / 255.0;
    darkness += dither * 2.0;

    // 合成
    float3 blendFactor = lerp(float3(1.0, 1.0, 1.0), gData.vignetteColor.rgb, darkness);

    return color * blendFactor;
}

// スキャンライン
float3 ApplyScanline(float3 color, float2 uv)
{
    float input = 0;
    if (gData.scanlineDirection == 0)
        input = uv.y;
    else if (gData.scanlineDirection == 1)
        input = uv.x;
    else
        input = (uv.x + uv.y) * 0.7071;

    float wave = sin((input * gData.scanlineFrequency + gData.totalTime * gData.scanlineScrollSpeed) * 6.28318);
    float mask = (wave + 1.0) * 0.5;
    return lerp(color, gData.scanlineColor, mask * gData.scanlineIntensity);
}

// フィルムグレイン
float3 ApplyFilmGrain(float3 color, float2 uv)
{
    float grain = noise(uv, gData.totalTime) - 0.5;
    float3 grainColor = float3(
        grain * (0.9 + 0.2 * random(uv * 10.0 + float2(1.0, 0.0))),
        grain * (0.9 + 0.2 * random(uv * 10.0 + float2(0.0, 1.0))),
        grain * (0.9 + 0.2 * random(uv * 10.0 + float2(1.0, 1.0)))
    );
    return saturate(color + grainColor * gData.filmGrainIntensity);
}

// =======================================================
//  特殊サンプリング
// =======================================================

// グリッチ
float4 SampleGlitch(float2 uv)
{
    float blockHeight = gData.glitchBlockHeight;
    float blockIndex = floor(uv.y / blockHeight);
    float timePhase = gData.totalTime * 0.5;

    // ノイズ生成
    float glitchOffsetX = (FBM(uv * 10.0 + float2(timePhase, 0.0), 4, 0.5, 2.0) - 0.5) * gData.glitchAmount;
    float glitchOffsetY = (FBM(uv * 10.0 + float2(0.0, timePhase), 4, 0.5, 2.0) - 0.5) * gData.glitchAmount * 0.3;
    float glitchSwitch = step(0.5, random(float2(blockIndex, floor(gData.totalTime * 5.0))));

    float2 glitchUV = uv + float2(glitchOffsetX, glitchOffsetY) * glitchSwitch;
    
    // 色収差的なズレ
    float2 offsetR = float2(0.003 * sin(gData.totalTime), 0.0);
    float2 offsetG = float2(-0.003 * cos(gData.totalTime), 0.0);
    float2 offsetB = float2(0.003 * sin(uv.y * 50.0 + gData.totalTime), 0.0);

    float r = gTexture.Sample(gSampler, glitchUV + offsetR).r;
    float g = gTexture.Sample(gSampler, glitchUV + offsetG).g;
    float b = gTexture.Sample(gSampler, glitchUV + offsetB).b;

    float3 col = float3(r, g, b);
    
    // ホワイトノイズ追加
    float n = (whiteNoise(uv * gData.screenResolution.xy + gData.totalTime * 100.0) - 0.5) * gData.glitchNoiseIntensity;
    return float4(saturate(col + n), 1.0);
}

// RGBスプリット
float4 SampleRGBSplit(float2 uv)
{
    float2 offset = float2(gData.rgbSplitOffset, 0);
    float r = gTexture.Sample(gSampler, saturate(uv - offset)).r;
    float g = gTexture.Sample(gSampler, uv).g;
    float b = gTexture.Sample(gSampler, saturate(uv + offset)).b;
    return float4(r, g, b, 1.0);
}

// クロマティックアベレーション
float4 SampleChromAb(float2 uv)
{
    float2 offset = gData.chromaOffset / gData.screenResolution;
    float r = gTexture.Sample(gSampler, uv + offset).r;
    float g = gTexture.Sample(gSampler, uv).g;
    float b = gTexture.Sample(gSampler, uv - offset).b;
    return float4(r, g, b, 1.0);
}

// ラディアルブラー
float4 ApplyRadialBlur(float2 uv)
{
    // サンプリング回数
    const int SAMPLES = 12;
    
    float4 color = float4(0, 0, 0, 0);
    
    // 中心から外側へ向かってサンプリング位置をずらしながら加算
    for (int i = 0; i < SAMPLES; i++)
    {
        // 0.0(中心) ～ 1.0(元の位置) の間でスケールを変化させる
        float scale = 1.0 - gData.radialBlurStrength * (float(i) / (float(SAMPLES) - 1));
        
        // 中心を基準にUVを縮小
        float2 sampleUV = (uv - gData.radialBlurCenter) * scale + gData.radialBlurCenter;
        
        // テクスチャサンプリングして加算
        color += gTexture.Sample(gSampler, sampleUV);
    }
    
    // 合計値を回数で割って平均化
    return color / float(SAMPLES);
}

// ディゾルブ
float4 ApplyDissolve(float4 currentColor, float2 uv)
{
    // ノイズテクスチャからサンプリング
    float noise = gDissolveTexture.Sample(gSampler, uv).r;

    // 閾値より低い部分は黒色にさせる
    if (noise <= gData.dissolveThreshold)
    {
        return float4(0.0f, 0.0f, 0.0f, 1.0f);
    }

    // 境界線の発光処理
    float thresholdEdge = gData.dissolveThreshold + gData.dissolveEdgeWidth;
    
    // ノイズ値が閾値 ～ 閾値+幅の間にある場合、エッジ色を適用
    if (noise < thresholdEdge)
    {
        // 閾値に近いほど強く発光させる係数
        float t = 1.0f - ((noise - gData.dissolveThreshold) / gData.dissolveEdgeWidth);
        
        // 芯が白く飛び、周囲がカラーになり、Bloomっぽく
        float glowFactor = pow(t, 2.5f) * gData.dissolveEdgeIntensity;
        
        // 加算合成
        float3 edgeGlow = gData.dissolveEdgeColor * glowFactor;

        // 加算合成のような見た目にする
        currentColor.rgb += edgeGlow;
        
        // 燃え尽きるように元絵を消す
        currentColor.rgb = lerp(currentColor.rgb, gData.dissolveEdgeColor * gData.dissolveEdgeIntensity, t);
    }

    return currentColor;
}

// LUTを使ったカラーグレーディング
float3 ApplyColorGradingLUT(float3 color)
{
    // LUTのサイズ
    const float LUT_SIZE = 32.0f;
    const float MAX_COLOR = LUT_SIZE - 1.0f;

    // 入力カラーをLUTのインデックスに変換 (0.0～1.0 -> 0.0～31.0)
    float3 lutIndex = saturate(color) * MAX_COLOR;

    // 2D展開されたLUT（1024x32）からサンプリングするためのUV計算
    float sliceX = floor(lutIndex.z); // 現在のスライス
    float nextSliceX = min(sliceX + 1.0f, MAX_COLOR); // 次のスライス（補間用）

    // ピクセル中心に合わせるためのハーフピクセルオフセット
    float halfPixelX = 0.5f / (LUT_SIZE * LUT_SIZE);
    float halfPixelY = 0.5f / LUT_SIZE;

    // 現在のスライスのUV
    float2 uv1;
    uv1.x = (sliceX * LUT_SIZE + lutIndex.x + 0.5f) / (LUT_SIZE * LUT_SIZE);
    uv1.y = (lutIndex.y + 0.5f) / LUT_SIZE;

    // 次のスライスのUV
    float2 uv2;
    uv2.x = (nextSliceX * LUT_SIZE + lutIndex.x + 0.5f) / (LUT_SIZE * LUT_SIZE);
    uv2.y = (lutIndex.y + 0.5f) / LUT_SIZE;

    // 2つのスライスからサンプリング
    float3 color1 = gLutTexture.SampleLevel(gClampSampler, uv1, 0).rgb;
    float3 color2 = gLutTexture.SampleLevel(gClampSampler, uv2, 0).rgb;

    // Z軸（青成分）の端数で線形補間
    float zFraction = frac(lutIndex.z);
    return lerp(color1, color2, zFraction);
}

float4 main(VSOutput input) : SV_TARGET
{
    float2 uv = input.uv;
    float4 finalColor = float4(0, 0, 0, 1);

    // 座標系を加工して視覚効果を作る
    if (gData.flag[0] & PIXELATION)
        uv = ApplyPixelation(uv);
    if (gData.flag[0] & SCREEN_WAVE)
        uv = ApplyScreenWave(uv);
    if (gData.flag[0] & HEAT_HAZE)
        uv = ApplyHeatHaze(uv);
    if (gData.flag[0] & WATER_REFRACTION)
        uv = ApplyWaterRefraction(uv);
    
    // 魚眼レンズ（範囲外黒塗りつぶしの判定）
    if (gData.flag[0] & FISHEYE)
    {
        uv = ApplyFisheye(uv);
        if (any(uv < 0.0) || any(uv > 1.0))
            return float4(0, 0, 0, 1);
    }

    // UVを元にテクスチャ取得
    if (gData.flag[0] & GLITCH)
    {
        finalColor = SampleGlitch(uv);
    }
    else if (gData.flag[0] & RGB_SPLIT)
    {
        finalColor = SampleRGBSplit(uv);
    }
    else if (gData.flag[0] & CHROM_ABERRATION)
    {
        finalColor = SampleChromAb(uv);
    }
    else if (gData.flag[0] & RADIAL_BLUR)
    {
        finalColor = ApplyRadialBlur(uv);
    }
    else
    {
        // 標準サンプリング
        finalColor = gTexture.Sample(gSampler, uv);
    }
    
    if (gData.flag[0] & DISSOLVE)
    {
        finalColor = ApplyDissolve(finalColor, uv);
    }

      // 色補正・フィルタ処理
    if (gData.flag[0] & GRAYSCALE)
        finalColor.rgb = ApplyGrayscale(finalColor.rgb);
    if (gData.flag[0] & SEPIA)
        finalColor.rgb = ApplySepia(finalColor.rgb);
    if (gData.flag[0] & COLOR_TINT)
        finalColor.rgb = ApplyColorTint(finalColor.rgb);
    if (gData.flag[0] & SCANLINE)
        finalColor.rgb = ApplyScanline(finalColor.rgb, uv);
    if (gData.flag[0] & VIGNETTE)
        finalColor.rgb = ApplyVignette(finalColor.rgb, uv);
    if (gData.flag[0] & FILM_GRAIN)
        finalColor.rgb = ApplyFilmGrain(finalColor.rgb, uv);

    // 単純加算・乱数系の処理
    if (gData.flag[0] & BLOCK_NOISE)
    {
        float2 blockSize = float2(gData.blockNoiseSize, gData.blockNoiseSize);
        float2 blockUV = floor(uv * gData.screenResolution / blockSize);

        float2 timeOffset = frac(gData.totalTime * gData.blockNoiseSpeed * float2(17.0, 23.0));

        float n = random(blockUV + timeOffset);

        float3 noiseColor = float3(n, n, n);
        finalColor.rgb = lerp(finalColor.rgb, noiseColor, gData.blockNoiseAmount);

    }
    
    if (gData.flag[0] & SCREEN_NOISE)
    {
        float2 scaledUV = uv * gData.screenResolution.xy * gData.noiseScale;

        float2 timeOffset = float2(gData.totalTime * gData.noiseSpeed, gData.totalTime * gData.noiseSpeed * 1.7);

        float nR = sin(dot(scaledUV + timeOffset.xy, float2(12.9898, 78.233))) * 43758.5453;
        float nG = sin(dot(scaledUV + timeOffset.xy + 3.0, float2(12.9898, 78.233))) * 43758.5453;
        float nB = sin(dot(scaledUV + timeOffset.xy + 6.0, float2(12.9898, 78.233))) * 43758.5453;

        nR = frac(nR);
        nG = frac(nG);
        nB = frac(nB);

        float3 noiseColor = float3(nR, nG, nB);
        finalColor.rgb += (noiseColor - 0.5) * gData.noiseAmount;
    }
    if (gData.flag[0] & COLOR_GRADING_LUT)
    {
        finalColor.rgb = ApplyColorGradingLUT(finalColor.rgb);
    }

    return finalColor;
}