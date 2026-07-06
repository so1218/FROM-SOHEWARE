#include "ShaderConstants.hlsli"

Texture2D<float4> gCurrentSSR : register(t0); // 空間デノイズ(Spatial)後の今フレームのSSR
Texture2D<float4> gHistorySSR : register(t1); // 前フレームのTemporal結果
Texture2D<float2> gVelocityTexture : register(t2); // G-Buffer: モーションベクトル (画面UV空間の移動量)

RWTexture2D<float4> gOutTemporalSSR : register(u0);

SamplerState gLinearSampler : register(s0);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint width, height;
    gOutTemporalSSR.GetDimensions(width, height);
    if (DTid.x >= width || DTid.y >= height)
        return;

    float2 uv = (float2(DTid.xy) + 0.5f) / float2(width, height);
    float4 currentColor = gCurrentSSR.Load(int3(DTid.xy, 0));
    float2 velocity = gVelocityTexture.Load(int3(DTid.xy, 0));
    float2 prevUV = uv - velocity;

    if (any(prevUV < 0.0f) || any(prevUV > 1.0f))
    {
        gOutTemporalSSR[DTid.xy] = currentColor;
        return;
    }

    float4 historyColor = gHistorySSR.SampleLevel(gLinearSampler, prevUV, 0);

   // ▼ 修正点: Variance Clipping (分散クリッピング)
    // 3x3ピクセルの色を集計して、平均値と分散（ばらつき）を計算する
    float4 m1 = float4(0, 0, 0, 0); // 色の合計
    float4 m2 = float4(0, 0, 0, 0); // 色の二乗の合計
    
  [unroll]
    for (int y = -1; y <= 1; ++y)
    {
        [unroll]
        for (int x = -1; x <= 1; ++x)
        {
            int2 neighborCoord = clamp(int2(DTid.xy) + int2(x, y), int2(0, 0), int2(width - 1, height - 1));
            float4 neighbor = gCurrentSSR.Load(int3(neighborCoord, 0));
            
            m1 += neighbor;
            m2 += neighbor * neighbor;
        }
    }
    
    // 平均 (Mean)
    float4 mean = m1 / 9.0f;
    // 分散 (Variance) = 二乗の平均 - 平均の二乗
    float4 variance = max(m2 / 9.0f - mean * mean, 0.0f);
    // 標準偏差 (Standard Deviation)
    float4 stddev = sqrt(variance);

    // ガンマ値（1.0〜1.5程度。値が大きいほど履歴を許容し残像が出やすいがノイズは減る）
    float gamma = 1.0f;
    
    // 統計学的に「正しい色の範囲」を定義
    float4 boxMin = mean - gamma * stddev;
    float4 boxMax = mean + gamma * stddev;
    
    // 履歴カラーを統計的な範囲内に収める（フリッカーが激減する）
    historyColor = clamp(historyColor, boxMin, boxMax);

    // アルファの乖離チェック (History Validation)
    float blendAlpha = 0.1f; // 基本は過去を90%信頼
    float alphaDiff = abs(currentColor.a - historyColor.a);
    
    // 反射が急に出現・消失した場所（差が50%以上）は履歴を信用せず今フレームを採用
    if (alphaDiff > 0.5f)
    {
        blendAlpha = 1.0f;
    }

    float4 finalSSR = lerp(historyColor, currentColor, blendAlpha);
    gOutTemporalSSR[DTid.xy] = finalSSR;
}