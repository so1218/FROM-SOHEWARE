// 書き込み用の3Dテクスチャ
RWTexture3D<float4> gOutputNoise : register(u0);

// =======================================================
// 1. 乱数生成関数 (Hash)
// =======================================================
// 入力された3D座標から、固定のランダムな方向ベクトル(-1.0 ~ 1.0)を作る
float3 hash33(float3 p)
{
    p = float3(dot(p, float3(127.1f, 311.7f, 74.7f)),
               dot(p, float3(269.5f, 183.3f, 246.1f)),
               dot(p, float3(113.5f, 271.9f, 124.6f)));
    return -1.0f + 2.0f * frac(sin(p) * 43758.5453123f);
}

// =======================================================
// 2. シームレスな 3D Perlin Noise (-1.0 ～ 1.0)
// =======================================================
// period（周期）を渡すことで、指定した回数でピタッとループするようにする
float PerlinNoise3D_Seamless(float3 p, float period)
{
    float3 pi = floor(p);
    float3 pf = p - pi;
    
    // Ken Perlinによる補間関数 (Fade) : 6t^5 - 15t^4 + 10t^3
    float3 w = pf * pf * pf * (pf * (pf * 6.0f - 15.0f) + 10.0f);
    
    // 周期(period)でラップすることで、端と端が繋がるシームレスなノイズにする
    float3 p000 = fmod(pi + float3(0, 0, 0), period);
    float3 p100 = fmod(pi + float3(1, 0, 0), period);
    float3 p010 = fmod(pi + float3(0, 1, 0), period);
    float3 p110 = fmod(pi + float3(1, 1, 0), period);
    float3 p001 = fmod(pi + float3(0, 0, 1), period);
    float3 p101 = fmod(pi + float3(1, 0, 1), period);
    float3 p011 = fmod(pi + float3(0, 1, 1), period);
    float3 p111 = fmod(pi + float3(1, 1, 1), period);

    // キューブの8つの頂点における影響度を計算
    float c000 = dot(hash33(p000), pf - float3(0, 0, 0));
    float c100 = dot(hash33(p100), pf - float3(1, 0, 0));
    float c010 = dot(hash33(p010), pf - float3(0, 1, 0));
    float c110 = dot(hash33(p110), pf - float3(1, 1, 0));
    float c001 = dot(hash33(p001), pf - float3(0, 0, 1));
    float c101 = dot(hash33(p101), pf - float3(1, 0, 1));
    float c011 = dot(hash33(p011), pf - float3(0, 1, 1));
    float c111 = dot(hash33(p111), pf - float3(1, 1, 1));

    // X, Y, Z方向への線形補間
    float x00 = lerp(c000, c100, w.x);
    float x10 = lerp(c010, c110, w.x);
    float x01 = lerp(c001, c101, w.x);
    float x11 = lerp(c011, c111, w.x);
    
    float y0 = lerp(x00, x10, w.y);
    float y1 = lerp(x01, x11, w.y);
    
    return lerp(y0, y1, w.z);
}

// =======================================================
// 3. フラクタル・ノイズ (fBm) の生成 (0.0 ～ 1.0)
// =======================================================
// --- 3. 改良されたフラクタル・ノイズ (fBm) ---
float CalculateComplexPerlinNoise(float3 uvw)
{
    float noise = 0.0f;
    float amplitude = 0.5f;
    float frequency = 4.0f;
    float maxAmplitude = 0.0f;

    // ★改善1: オフセット用のテーブル（適当な大きな素数に近い値）
    float3 offsets[4] =
    {
        float3(0.0f, 0.0f, 0.0f),
        float3(135.31f, 250.74f, 180.11f),
        float3(311.13f, 15.25f, 95.82f),
        float3(67.91f, 340.21f, 210.56f)
    };

    // ★改善2: ドメイン・ワーピング (座標をノイズで歪ませる)
    // これを入れるだけで「雲」や「煙」の質感が劇的に向上します
    float3 warp = float3(
        PerlinNoise3D_Seamless(uvw * 2.0f, 2.0f),
        PerlinNoise3D_Seamless(uvw * 2.0f + 15.3f, 2.0f),
        PerlinNoise3D_Seamless(uvw * 2.0f + 31.1f, 2.0f)
    );
    // 歪み具合を調整 (0.1f ～ 0.2f 程度が自然)
    uvw += warp * 0.15f;

    const int OCTAVES = 4;
    for (int i = 0; i < OCTAVES; i++)
    {
        // ★改善3: 軸の入れ替えとオフセットの適用
        // 周期(frequency)でシームレス性を保ちつつ、座標をバラバラにする
        float3 p = uvw * frequency + offsets[i];
        
        // オクターブごとに軸を回転させてパターンの重なりを壊す
        if (i == 1)
            p = p.yzx;
        if (i == 2)
            p = p.zxy;
        if (i == 3)
            p = p.yxz;

        float perlin = PerlinNoise3D_Seamless(p, frequency);
        
        // 振幅の加算
        noise += (perlin * 0.5f + 0.5f) * amplitude;
        maxAmplitude += amplitude;
        
        amplitude *= 0.5f;
        frequency *= 2.0f;
    }
    
    return noise / maxAmplitude;
}
// =======================================================
// 新設: シームレスな 3D Worley Noise (0.0 ～ 1.0)
// =======================================================
float WorleyNoise3D_Seamless(float3 p, float period)
{
    float3 pi = floor(p);
    float3 pf = p - pi;
    float minDist = 1.0f;

    // 周囲27個のセルを探索
    for (int z = -1; z <= 1; z++)
    {
        for (int y = -1; y <= 1; y++)
        {
            for (int x = -1; x <= 1; x++)
            {
                float3 offset = float3(x, y, z);
                // 周期でラップしてシームレス化（負の数対策で + period）
                float3 cell = fmod(pi + offset + period, period);
                
                // セル内のランダムな中心点 (0.0 ～ 1.0)
                float3 cellPoint = hash33(cell) * 0.5f + 0.5f;
                
                float3 diff = offset + cellPoint - pf;
                float dist = length(diff);
                minDist = min(minDist, dist);
            }
        }
    }
    return saturate(minDist);
}

[numthreads(8, 8, 8)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint width, height, depth;
    gOutputNoise.GetDimensions(width, height, depth);
    if (DTid.x >= width || DTid.y >= height || DTid.z >= depth)
        return;

    float3 uvw = float3(DTid) / float3(width, height, depth);

    // 1. Rチャンネル: 既存のPerlin(fBm)ノイズ
    float perlinValue = CalculateComplexPerlinNoise(uvw);
    perlinValue = pow(perlinValue, 1.2f);
    perlinValue = smoothstep(0.15f, 0.85f, perlinValue);

    // 2. Gチャンネル: Worleyノイズ（密度の削り・もこもこディテール用）
    float worley1 = WorleyNoise3D_Seamless(uvw * 4.0f, 4.0f);
    float worley2 = WorleyNoise3D_Seamless(uvw * 8.0f, 8.0f);
    
    // ★修正：反転（1.0 - W）させて「もこもこの塊」にしてから合成し、最後にまた反転して戻す
    // これにより、クレーターの底（0.0）が綺麗に維持され、かつ細かいディテールが刻まれます
    float fbmWorley = (1.0f - worley1) * 0.6f + (1.0f - worley2) * 0.4f;
    float worleyValue = saturate(1.0f - fbmWorley);

    // ※もし上記でもフォグが薄すぎる場合は、より谷がハッキリ残る「min合成」を試してください
    // float worleyValue = saturate(min(worley1, worley2 * 1.5f));

    // RGBAテクスチャに別々に保存！
    gOutputNoise[DTid] = float4(perlinValue, worleyValue, 0.0f, 1.0f);
}