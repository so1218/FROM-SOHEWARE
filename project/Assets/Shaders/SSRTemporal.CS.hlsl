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

    // 1. 今フレームのカラーを取得
    float4 currentColor = gCurrentSSR.Load(int3(DTid.xy, 0));

    // 2. モーションベクトルを取得して、前フレームのUVを計算
    float2 velocity = gVelocityTexture.Load(int3(DTid.xy, 0));
    float2 prevUV = uv - velocity; // 今のUVから移動量を引いて過去のUVへ

    // 画面外に出ていれば、履歴を使わずに今の色を出力して終了
    if (any(prevUV < 0.0f) || any(prevUV > 1.0f))
    {
        gOutTemporalSSR[DTid.xy] = currentColor;
        return;
    }

    // 3. 過去のカラーを取得 (リニアサンプリングで滑らかに)
    float4 historyColor = gHistorySSR.SampleLevel(gLinearSampler, prevUV, 0);

    // 4. Neighborhood Clamping (残像・ゴースト軽減)
    // Fogのコードと同様に、現在のピクセルの周囲3x3の最大色・最小色を求め、過去の色をクランプする
    float4 boxMin = currentColor;
    float4 boxMax = currentColor;
    
    // ※軽量化のために十字(5タップ)や3x3(9タップ)を使用
    int2 offsets[4] = { int2(-1, 0), int2(1, 0), int2(0, -1), int2(0, 1) };
    for (int i = 0; i < 4; ++i)
    {
        int2 neighborCoord = clamp(int2(DTid.xy) + offsets[i], int2(0, 0), int2(width - 1, height - 1));
        float4 neighbor = gCurrentSSR.Load(int3(neighborCoord, 0));
        boxMin = min(boxMin, neighbor);
        boxMax = max(boxMax, neighbor);
    }
    
    // 過去の色が、現在の周囲の色から突飛に離れていたらクランプ（ゴーストを消す）
    historyColor = clamp(historyColor, boxMin, boxMax);

    // 5. ブレンド (TAAウェイト)
    // 基本は過去を強く(90%〜95%)信じて蓄積し、滑らかにする
    float blendAlpha = 0.1f;
    
    float4 finalSSR = lerp(historyColor, currentColor, blendAlpha);

    gOutTemporalSSR[DTid.xy] = finalSSR;
}