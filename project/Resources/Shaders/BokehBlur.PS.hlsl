#include "FullScreenQuad.hlsli"
#include "ShaderConstants.hlsli"

Texture2D gSceneTexture : register(t0);
Texture2D<float> gDepthTexture : register(t1);
SamplerState gSampler : register(s0);

ConstantBuffer<DoFSettingsData> gDoFSettings : register(b0);

// フレーム全体の共通データ（Near/Far, 解像度など）
// C++側で slot 1 にバインドしてください
ConstantBuffer<FrameData> gFrameData : register(b1);

// 深度リニア化
float LinearizeDepth(float d)
{
    float n = gFrameData.nearClip;
    float f = gFrameData.farClip;
    return (n * f) / (f - d * (f - n));
}

// 符号付きCoCの計算
// 負 = 手前ボケ, 正 = 奥ボケ
float GetSignedCoC(float depth)
{
    // 物理ベースに近い計算
    float coc = (depth - gDoFSettings.focusDistance) / max(0.0001f, depth);
    
    // 手前と奥でボケの強さを変えられるように係数を分けるのがプロのテクニック
    // 例: 手前は強烈にボカしたい場合
    float scale = (depth < gDoFSettings.focusDistance) ? 1.0f : 1.0f;
    
    // ピクセル単位ではなく、正規化されたCoC(-1.0 ~ 1.0)を返す
    // focusRangeでボケの感度を調整
    float factor = 100.0f / max(0.1f, gDoFSettings.focusRange);
    return clamp(coc * factor * scale, -1.0f, 1.0f);
}

// 黄金角
static const float GOLDEN_ANGLE = 2.39996323f;
static const int SAMPLE_COUNT = 64;

float4 main(VSOutput input) : SV_TARGET
{
    float2 uv = input.uv;
    
    // 1. 中心ピクセルの情報取得
    float centerDepth = LinearizeDepth(gDepthTexture.Sample(gSampler, uv));
    float centerCoC = GetSignedCoC(centerDepth); // 符号付き(-1 ~ 1)
    float centerAbsCoC = abs(centerCoC);

    // ★最適化: ほとんどボケていないなら早期リターン
    // ただし、周囲から「前ボケ」が浸食してくる可能性があるので、
    // 完全に0でリターンすると前ボケの境界が不自然になる。
    // 高品質を目指すなら、少し余裕を持たせるか、あえてスキップしない。
    if (centerAbsCoC < 0.001f)
    {
        // ここでリターンせず、前ボケスキャン（近傍探索）をするのが本当の最高品質ですが、
        // 重すぎるため今回は「自分がボケていれば処理」に進む
    }

    float3 finalColor = float3(0, 0, 0);
    float totalWeight = 0.0f;
    
    float aspect = (gDoFSettings.resolution.x / 2.0f) / (gDoFSettings.resolution.y / 2.0f);

    // ★色収差用のオフセット（ボケの縁に色ズレを作る）
    float3 chromaticScale = float3(1.0f, 0.98f, 0.96f); // R, G, B でサンプリング半径を微妙に変える

    // ----------------------------------------------------
    // ボケ・サンプリングループ
    // ----------------------------------------------------
    for (int i = 0; i < SAMPLE_COUNT; i++)
    {
        float theta = i * GOLDEN_ANGLE;
        float r = sqrt((float) i) / sqrt((float) SAMPLE_COUNT);
        
        // 絞り形状のシミュレーション（円形ではなく多角形にするなど）
        // ここでは単純な円形だが、r を pow(r, 0.8) などにすると周辺光量が落ちてリアルになる
        
        float2 offsetBase = float2(cos(theta), sin(theta)) * r * gDoFSettings.bokehRadius;
        offsetBase.x /= aspect;
        offsetBase /= gDoFSettings.resolution.xy / 2.0f;

        // ★最大半径でサンプリング位置を決定
        float2 sampleUV = uv + offsetBase;

        // サンプリング（色と深度）
        float3 sampleColor = gSceneTexture.SampleLevel(gSampler, sampleUV, 0).rgb;
        float sampleDepth = LinearizeDepth(gDepthTexture.SampleLevel(gSampler, sampleUV, 0));
        float sampleCoC = GetSignedCoC(sampleDepth); // 符号付き
        float sampleAbsCoC = abs(sampleCoC);

        // -----------------------------------------------------------
        // ★【最重要】ウェイト計算（Foreground Bleeding対応）
        // -----------------------------------------------------------
        float weight = 1.0f;

        // ケースA: 「奥ボケ」の計算 (sampleDepth > centerDepth)
        // 中心より奥にある画素は、中心がボケている場合のみ混ざる。
        if (sampleDepth > centerDepth)
        {
            weight = saturate(centerAbsCoC);
        }
        // ケースB: 「手前ボケ」の計算 (sampleDepth < centerDepth)
        // ここがポイント。「サンプリング点」が手前にあり、かつ「その点がボケている」なら、
        // 中心点がピント合っていても、手前のボケが覆いかぶさる必要がある。
        else
        {
            // サンプリング点のボケ半径が、現在の距離まで届いているか？
            // 簡易判定: サンプリング点のCoCが大きいほどウェイトを大きく
            weight = saturate(sampleAbsCoC * 2.0f);
        }

        // 近すぎるサンプリング点の重複排除（Tweak）
        // 完全に重なっているピクセルのウェイト調整
        
        // -----------------------------------------------------------
        // ★ボケの質感を高める (Bokeh Highlights)
        // -----------------------------------------------------------
        float luminance = dot(sampleColor, float3(0.299, 0.587, 0.114));
        // 閾値を超えた明るい部分を強調（玉ボケを作る）
        float boost = smoothstep(0.7f, 1.0f, luminance) * 4.0f + 1.0f;
        weight *= boost;

        // -----------------------------------------------------------
        // ★色収差 (Chromatic Aberration) の適用
        // -----------------------------------------------------------
        // 本当はRGBごとにUVをずらして3回Sampleするのが正確ですが重いので、
        // 擬似的に色に重み付けをして「縁」に色をつける
        float3 colorWeight = float3(weight, weight, weight);
        
        // サンプリング位置が「外側」に行くほど色を分離させる
        if (r > 0.5f)
        {
            // わざとRGBのウェイトをずらす
            colorWeight *= float3(1.2f, 1.0f, 0.8f);
        }

        finalColor += sampleColor * colorWeight;
        totalWeight += (colorWeight.r + colorWeight.g + colorWeight.b) / 3.0f;
    }

    // ウェイトが0なら元の色を返す（ゼロ除算防止）
    if (totalWeight < 0.001f)
    {
        return gSceneTexture.Sample(gSampler, uv);
    }

    return float4(finalColor / totalWeight, 1.0f);
}