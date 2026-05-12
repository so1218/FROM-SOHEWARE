// 書き込み用の3Dテクスチャ
RWTexture3D<float> gOutputNoise : register(u0);

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
float CalculateComplexPerlinNoise(float3 uvw)
{
    float noise = 0.0f;
    float amplitude = 0.5f; // 影響力（最初は50%）
    float frequency = 4.0f; // テクスチャ全体の繰り返し回数（初期4回）
    float maxAmplitude = 0.0f; // 正規化用
    
    const int OCTAVES = 4; // ノイズを重ねる回数（4回で十分綺麗です）

    for (int i = 0; i < OCTAVES; i++)
    {
        // PerlinNoiseは -1～1 で返ってくるので、0～1 に直して加算
        float perlin = PerlinNoise3D_Seamless(uvw * frequency, frequency);
        noise += (perlin * 0.5f + 0.5f) * amplitude;
        
        maxAmplitude += amplitude;
        
        // 次のループに向けて、影響力を半分にし、細かさを倍にする
        amplitude *= 0.5f;
        frequency *= 2.0f;
    }
    
    // 全体を 0.0 ～ 1.0 に収めて返す
    return noise / maxAmplitude;
}

// =======================================================
// エントリーポイント
// =======================================================
[numthreads(8, 8, 8)] // ★3DなのでZ方向も8スレッド
void main(uint3 DTid : SV_DispatchThreadID)
{
    // テクスチャのサイズを取得（例: 64x64x64）
    uint width, height, depth;
    gOutputNoise.GetDimensions(width, height, depth);
    
    if (DTid.x >= width || DTid.y >= height || DTid.z >= depth)
        return;

    // 現在のピクセル位置を 0.0 ～ 1.0 のUV座標(UVW)に変換
    float3 uvw = float3(DTid) / float3(width, height, depth);

    // 複雑なノイズを計算
    float noiseValue = CalculateComplexPerlinNoise(uvw);
    
    // 少しコントラストをつけてメリハリを出す（お好みで調整可）
    noiseValue = smoothstep(0.2f, 0.8f, noiseValue);

    // 3Dテクスチャに書き込み
    gOutputNoise[DTid] = noiseValue;
}