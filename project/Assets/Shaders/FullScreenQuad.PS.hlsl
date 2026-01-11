#include "FullScreenQuad.hlsli"

Texture2D gTexture : register(t0);
Texture2D gDissolveTexture : register(t1);
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

    float chromaOffset;
    float waveAmplitude;
    float waveFrequency;
    float _paddingWave;

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
    
    float dissolveThreshold; 
    float dissolveEdgeWidth; 
    float dissolveEdgeIntensity; 
    float _paddingDissolve;

    float3 dissolveEdgeColor; 
    float _paddingDissolve2; 
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
    float2 pixelSizeUV = pixelationSize / screenResolution;
    return floor(uv / pixelSizeUV) * pixelSizeUV;
}

// 画面波（横・縦・両方）
float2 ApplyScreenWave(float2 uv)
{
    float2 waveOffset = float2(0.0, 0.0);
    // 横波
    if (waveDirection == 0 || waveDirection == 2)
    {
        waveOffset.x += sin(uv.y * waveFrequency + totalTime * waveSpeed) * waveAmplitude;
    }
    // 縦波
    if (waveDirection == 1 || waveDirection == 2)
    {
        waveOffset.y += sin(uv.x * waveFrequency + totalTime * waveSpeed) * waveAmplitude;
    }

    // 画面端のフェード処理（端で歪まないように）
    float edgeFade = saturate((1.0 - abs(uv.x - 0.5) * 2.0) * (1.0 - abs(uv.y - 0.5) * 2.0));
    edgeFade = smoothstep(0.0, 0.2, edgeFade);

    return saturate(uv + waveOffset * edgeFade);
}

// 陽炎
float2 ApplyHeatHaze(float2 uv)
{
    float2 noiseUV = uv * heatNoiseScale + float2(totalTime * heatSpeed, totalTime * heatSpeed * 0.3);
    float noiseX = FBM(noiseUV + float2(13.0, 7.0), 5, 0.5, 2.0);
    float noiseY = FBM(noiseUV + float2(21.0, 11.0), 5, 0.5, 2.0);
    float2 offset = (float2(noiseX, noiseY) - 0.5) * heatDistortionStrength;
    return saturate(uv + offset);
}

// 水面屈折
float2 ApplyWaterRefraction(float2 uv)
{
    float2 noiseUV = uv * turbulentFrequency + float2(totalTime * turbulentSpeed, 0.0);
    float displacement = sin(noiseUV.x * 10.0 + totalTime * 0.5) * 0.5 + sin(noiseUV.y * 10.0 + totalTime * 0.5) * 0.5;
    return uv + float2(displacement, displacement) * turbulentStrength;
}

// 魚眼レンズ
float2 ApplyFisheye(float2 uv)
{
    float2 center = float2(0.5, 0.5);
    float2 offset = uv - center;
    float dist = length(offset);
    float distDistorted = dist + fisheyeDistortion * dist * dist;
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
    return lerp(color, float3(gray, gray, gray), grayscaleColorAmount);
}

// セピア
float3 ApplySepia(float3 color)
{
    float3 sepia = float3(
        dot(color, float3(0.393, 0.769, 0.189)),
        dot(color, float3(0.349, 0.686, 0.168)),
        dot(color, float3(0.272, 0.534, 0.131))
    );
    return lerp(color, sepia, sepiaColorAmount);
}

// 色かぶり
float3 ApplyColorTint(float3 color)
{
    float3 mulTint = lerp(color, color * tintColor, tintMulColorAmount);
    float3 addTint = saturate(color + tintColor * tintAddColorAmount);
    float3 screenTint = 1.0 - (1.0 - color) * (1.0 - tintColor * tintScreenColorAmount);
    return (mulTint + addTint + screenTint) / 3.0;
}

// ビネット
float3 ApplyVignette(float3 color, float2 uv)
{
    // 中心座標
    float2 center = uv - 0.5;
    
    // 距離の計算
    float dist = length(center / vignetteEllipseScale);

    // 距離に基づいたリニアな進行度を計算
    // ゼロ除算対策
    float fadeLength = max(vignetteSoftness, 0.0001);
    
    float darkness = (dist - vignetteRadius) / fadeLength;
    
    // 0.0～1.0の範囲に切り取る
    darkness = saturate(darkness);

    // 滑らかにする
    darkness = smoothstep(0.0, 1.0, darkness);

    // 強度とディザリング
    darkness *= vignetteAmount;

    float dither = (random(uv) - 0.5) / 255.0;
    darkness += dither * 2.0;

    // 合成
    float3 blendFactor = lerp(float3(1.0, 1.0, 1.0), vignetteColor.rgb, darkness);

    return color * blendFactor;
}

// スキャンライン
float3 ApplyScanline(float3 color, float2 uv)
{
    float input = 0;
    if (scanlineDirection == 0)
        input = uv.y;
    else if (scanlineDirection == 1)
        input = uv.x;
    else
        input = (uv.x + uv.y) * 0.7071;

    float wave = sin((input * scanlineFrequency + totalTime * scanlineScrollSpeed) * 6.28318);
    float mask = (wave + 1.0) * 0.5;
    return lerp(color, scanlineColor, mask * scanlineIntensity);
}

// フィルムグレイン
float3 ApplyFilmGrain(float3 color, float2 uv)
{
    float grain = noise(uv, totalTime) - 0.5;
    float3 grainColor = float3(
        grain * (0.9 + 0.2 * random(uv * 10.0 + float2(1.0, 0.0))),
        grain * (0.9 + 0.2 * random(uv * 10.0 + float2(0.0, 1.0))),
        grain * (0.9 + 0.2 * random(uv * 10.0 + float2(1.0, 1.0)))
    );
    return saturate(color + grainColor * filmGrainIntensity);
}

// =======================================================
//  特殊サンプリング
// =======================================================

// グリッチ
float4 SampleGlitch(float2 uv)
{
    float blockHeight = glitchBlockHeight;
    float blockIndex = floor(uv.y / blockHeight);
    float timePhase = totalTime * 0.5;

    // ノイズ生成
    float glitchOffsetX = (FBM(uv * 10.0 + float2(timePhase, 0.0), 4, 0.5, 2.0) - 0.5) * glitchAmount;
    float glitchOffsetY = (FBM(uv * 10.0 + float2(0.0, timePhase), 4, 0.5, 2.0) - 0.5) * glitchAmount * 0.3;
    float glitchSwitch = step(0.5, random(float2(blockIndex, floor(totalTime * 5.0))));

    float2 glitchUV = uv + float2(glitchOffsetX, glitchOffsetY) * glitchSwitch;
    
    // 色収差的なズレ
    float2 offsetR = float2(0.003 * sin(totalTime), 0.0);
    float2 offsetG = float2(-0.003 * cos(totalTime), 0.0);
    float2 offsetB = float2(0.003 * sin(uv.y * 50.0 + totalTime), 0.0);

    float r = gTexture.Sample(gSampler, glitchUV + offsetR).r;
    float g = gTexture.Sample(gSampler, glitchUV + offsetG).g;
    float b = gTexture.Sample(gSampler, glitchUV + offsetB).b;

    float3 col = float3(r, g, b);
    
    // ホワイトノイズ追加
    float n = (whiteNoise(uv * screenResolution.xy + totalTime * 100.0) - 0.5) * glitchNoiseIntensity;
    return float4(saturate(col + n), 1.0);
}

// RGBスプリット
float4 SampleRGBSplit(float2 uv)
{
    float2 offset = float2(rgbSplitOffset, 0);
    float r = gTexture.Sample(gSampler, saturate(uv - offset)).r;
    float g = gTexture.Sample(gSampler, uv).g;
    float b = gTexture.Sample(gSampler, saturate(uv + offset)).b;
    return float4(r, g, b, 1.0);
}

// クロマティックアベレーション
float4 SampleChromAb(float2 uv)
{
    float2 offset = chromaOffset / screenResolution;
    float r = gTexture.Sample(gSampler, uv + offset).r;
    float g = gTexture.Sample(gSampler, uv).g;
    float b = gTexture.Sample(gSampler, uv - offset).b;
    return float4(r, g, b, 1.0);
}

// ディゾルブ
float4 ApplyDissolve(float4 currentColor, float2 uv)
{
    // ノイズテクスチャからサンプリング
    float noise = gDissolveTexture.Sample(gSampler, uv).r;

    // 閾値より低い部分は消滅（黒色）させる
    if (noise <= dissolveThreshold)
    {
        return float4(0.0f, 0.0f, 0.0f, 1.0f);
    }

    // 境界線の発光処理
    float thresholdEdge = dissolveThreshold + dissolveEdgeWidth;
    
    // ノイズ値が閾値 ～ 閾値+幅の間にある場合、エッジ色を適用
    if (noise < thresholdEdge)
    {
        // 閾値に近いほど強く発光させる係数
        float t = 1.0f - ((noise - dissolveThreshold) / dissolveEdgeWidth);
        
        // 芯が白く飛び、周囲がカラーになり、Bloomっぽくする
        float glowFactor = pow(t, 2.5f) * dissolveEdgeIntensity;
        
        // 加算合成
        float3 edgeGlow = dissolveEdgeColor * glowFactor;

        // 加算合成のような見た目にする
        currentColor.rgb += edgeGlow;
        
        // 燃え尽きるように元絵を消す
        currentColor.rgb = lerp(currentColor.rgb, dissolveEdgeColor * dissolveEdgeIntensity, t);
    }

    return currentColor;
}

float4 main(VSOutput input) : SV_TARGET
{
    float2 uv = input.uv;
    float4 finalColor = float4(0, 0, 0, 1);

    // 座標系を加工して視覚効果を作る
    if (flag.x & PIXELATION)
        uv = ApplyPixelation(uv);
    if (flag.x & SCREEN_WAVE)
        uv = ApplyScreenWave(uv);
    if (flag.x & HEAT_HAZE)
        uv = ApplyHeatHaze(uv);
    if (flag.x & WATER_REFRACTION)
        uv = ApplyWaterRefraction(uv);
    
    // 魚眼レンズ（範囲外黒塗りつぶしの判定）
    if (flag.x & FISHEYE)
    {
        uv = ApplyFisheye(uv);
        if (any(uv < 0.0) || any(uv > 1.0))
            return float4(0, 0, 0, 1);
    }

    // UVを元にテクスチャ取得
    if (flag.x & GLITCH)
    {
        finalColor = SampleGlitch(uv);
    }
    else if (flag.x & RGB_SPLIT)
    {
        finalColor = SampleRGBSplit(uv);
    }
    else if (flag.x & CHROM_ABERRATION)
    {
        finalColor = SampleChromAb(uv);
    }
    else
    {
        // 標準サンプリング
        finalColor = gTexture.Sample(gSampler, uv);
    }
    
    if (flag.x & DISSOLVE)
    {
        finalColor = ApplyDissolve(finalColor, uv);
    }

      // 色補正・フィルタ処理
    if (flag.x & GRAYSCALE)
        finalColor.rgb = ApplyGrayscale(finalColor.rgb);
    if (flag.x & SEPIA)
        finalColor.rgb = ApplySepia(finalColor.rgb);
    if (flag.x & COLOR_TINT)
        finalColor.rgb = ApplyColorTint(finalColor.rgb);
    if (flag.x & SCANLINE)
        finalColor.rgb = ApplyScanline(finalColor.rgb, uv);
    if (flag.x & VIGNETTE)
        finalColor.rgb = ApplyVignette(finalColor.rgb, uv);
    if (flag.x & FILM_GRAIN)
        finalColor.rgb = ApplyFilmGrain(finalColor.rgb, uv);

    // 単純加算・乱数系の処理
    if (flag.x & BLOCK_NOISE)
    {
        float2 blockSize = float2(blockNoiseSize, blockNoiseSize);
        float2 blockUV = floor(uv * screenResolution / blockSize);

        float2 timeOffset = frac(totalTime * blockNoiseSpeed * float2(17.0, 23.0));

        float n = random(blockUV + timeOffset);

        float3 noiseColor = float3(n, n, n);
        finalColor.rgb = lerp(finalColor.rgb, noiseColor, blockNoiseAmount);

    }
    
    if (flag.x & SCREEN_NOISE)
    {
        float2 scaledUV = uv * screenResolution.xy * noiseScale;

        float2 timeOffset = float2(totalTime * noiseSpeed, totalTime * noiseSpeed * 1.7);

        float nR = sin(dot(scaledUV + timeOffset.xy, float2(12.9898, 78.233))) * 43758.5453;
        float nG = sin(dot(scaledUV + timeOffset.xy + 3.0, float2(12.9898, 78.233))) * 43758.5453;
        float nB = sin(dot(scaledUV + timeOffset.xy + 6.0, float2(12.9898, 78.233))) * 43758.5453;

        nR = frac(nR);
        nG = frac(nG);
        nB = frac(nB);

        float3 noiseColor = float3(nR, nG, nB);
        finalColor.rgb += (noiseColor - 0.5) * noiseAmount;
    }

    return finalColor;
}