Texture2D<float4> gRawFogTexture : register(t0);
Texture2D<float> gDepthTexture : register(t1);

// --- 出力リソース (UAV) ---
// ぼかし処理が終わった最終結果
RWTexture2D<float4> gFilteredFog : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint width, height;
    gFilteredFog.GetDimensions(width, height);
    if (DTid.x >= width || DTid.y >= height)
        return;

    // 1. 中心ピクセルの情報を取得
    int3 centerPos = int3(DTid.xy, 0);
    float4 centerFog = gRawFogTexture.Load(centerPos);
    float centerDepth = gDepthTexture.Load(centerPos).r;

    float4 resultColor = float4(0, 0, 0, 0);
    float totalWeight = 0.0f;

    // ぼかし半径（固定値でテストする場合は 2 に設定）
    int radius = 2;
    
    // 空間ウェイト計算用の定数
    float spatialCoeff = 1.0f / (2.0f * 2.0f * 2.0f); // sigma = 2.0 の場合
    
    // 深度ウェイト計算用の定数（Zバッファの値は非線形なので、非常に小さな値にする必要があります）
    float depthCoeff = 1.0f / (2.0f * 0.001f * 0.001f); // sigma = 0.001 の場合

    // 2. 周辺ピクセルをサンプリングして合成（5x5のカーネル）
    for (int y = -radius; y <= radius; ++y)
    {
        for (int x = -radius; x <= radius; ++x)
        {
            // サンプル座標の計算（画面外にはみ出さないようにクランプ）
            int2 sampleCoord = clamp(int2(DTid.x + x, DTid.y + y), int2(0, 0), int2(width - 1, height - 1));
            int3 samplePos = int3(sampleCoord, 0);

            // 周辺ピクセルの情報を取得
            float4 sampleFog = gRawFogTexture.Load(samplePos);
            float sampleDepth = gDepthTexture.Load(samplePos).r;

            // --- ウェイト（重み）の計算 ---
            
            // A. 空間ウェイト（中心から遠いピクセルほど影響力を下げる）
            float distSq = (float) (x * x + y * y);
            float spatialWeight = exp(-distSq * spatialCoeff);

            // B. 深度ウェイト（中心ピクセルと深度が離れているほど影響力をゼロに近づける）
            // これにより、キャラクターの輪郭と背景の空が混ざる「光漏れ」を防ぎます
            float depthDiff = abs(centerDepth - sampleDepth);
            float depthWeight = exp(-(depthDiff * depthDiff) * depthCoeff);

            // 最終的なウェイト
            float weight = spatialWeight * depthWeight;

            // 結果に加算
            resultColor += sampleFog * weight;
            totalWeight += weight;
        }
    }

    // 3. 総ウェイトで割って平均化し、出力テクスチャに書き込む
    gFilteredFog[DTid.xy] = resultColor / max(totalWeight, 0.00001f);
}