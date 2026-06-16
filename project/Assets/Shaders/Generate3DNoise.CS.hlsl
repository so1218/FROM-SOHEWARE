// 書き込み用の3Dテクスチャ
RWTexture3D<float4> gOutputNoise : register(u0);

// 乱数生成関数 (Hash)
float3 hash33(float3 p3)
{
    p3 = frac(p3 * float3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yxz + 33.33);
    return -1.0f + 2.0f * frac((p3.xxy + p3.yxx) * p3.zyx);
}

// シームレスな 3D Perlin Noise (-1.0 ～ 1.0)
float PerlinNoise3D_Seamless(float3 p, float period)
{
    float3 pi = floor(p);
    float3 pf = p - pi;
    
    float3 w = pf * pf * pf * (pf * (pf * 6.0f - 15.0f) + 10.0f);
    
    float3 p000 = fmod(pi + float3(0, 0, 0), period);
    float3 p100 = fmod(pi + float3(1, 0, 0), period);
    float3 p010 = fmod(pi + float3(0, 1, 0), period);
    float3 p110 = fmod(pi + float3(1, 1, 0), period);
    float3 p001 = fmod(pi + float3(0, 0, 1), period);
    float3 p101 = fmod(pi + float3(1, 0, 1), period);
    float3 p011 = fmod(pi + float3(0, 1, 1), period);
    float3 p111 = fmod(pi + float3(1, 1, 1), period);

    float c000 = dot(hash33(p000), pf - float3(0, 0, 0));
    float c100 = dot(hash33(p100), pf - float3(1, 0, 0));
    float c010 = dot(hash33(p010), pf - float3(0, 1, 0));
    float c110 = dot(hash33(p110), pf - float3(1, 1, 0));
    float c001 = dot(hash33(p001), pf - float3(0, 0, 1));
    float c101 = dot(hash33(p101), pf - float3(1, 0, 1));
    float c011 = dot(hash33(p011), pf - float3(0, 1, 1));
    float c111 = dot(hash33(p111), pf - float3(1, 1, 1));

    float x00 = lerp(c000, c100, w.x);
    float x10 = lerp(c010, c110, w.x);
    float x01 = lerp(c001, c101, w.x);
    float x11 = lerp(c011, c111, w.x);
    
    float y0 = lerp(x00, x10, w.y);
    float y1 = lerp(x01, x11, w.y);
    
    return lerp(y0, y1, w.z);
}

// フラクタル・ノイズ (fBm) の生成 (0.0 ～ 1.0)
float CalculateComplexPerlinNoise(float3 uvw)
{
    float noise = 0.0f;
    float amplitude = 0.5f;
    
    float frequency = 1.5f;
    float maxAmplitude = 0.0f;

    float3 offsets[4] =
    {
        float3(0.0f, 0.0f, 0.0f),
        float3(135.31f, 250.74f, 180.11f),
        float3(311.13f, 15.25f, 95.82f),
        float3(67.91f, 340.21f, 210.56f)
    };

    // ドメイン・ワーピング
    float3 warp = float3(
        PerlinNoise3D_Seamless(uvw * 2.0f, 2.0f),
        PerlinNoise3D_Seamless(uvw * 2.0f + 15.3f, 2.0f),
        PerlinNoise3D_Seamless(uvw * 2.0f + 31.1f, 2.0f)
    );
    uvw += warp * 0.15f;

    const int OCTAVES = 4;
    for (int i = 0; i < OCTAVES; i++)
    {
        float3 p = uvw * frequency + offsets[i];
        
        if (i == 1)
            p = p.yzx;
        if (i == 2)
            p = p.zxy;
        if (i == 3)
            p = p.yxz;

        float perlin = PerlinNoise3D_Seamless(p, frequency);
        
        noise += (perlin * 0.5f + 0.5f) * amplitude;
        maxAmplitude += amplitude;
        
        amplitude *= 0.5f;
        frequency *= 2.0f;
    }
    
    return noise / maxAmplitude;
}

// シームレスな 3D Worley Noise (0.0 ～ 1.0)
float WorleyNoise3D_Seamless(float3 p, float period)
{
    float3 pi = floor(p);
    float3 pf = p - pi;
    float minDist = 1.0f;

    for (int z = -1; z <= 1; z++)
    {
        for (int y = -1; y <= 1; y++)
        {
            for (int x = -1; x <= 1; x++)
            {
                float3 offset = float3(x, y, z);
                float3 cell = fmod(pi + offset + period, period);
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

    // R: マクロ形状（ゆったりとした大きな雲のベース）
    float rPerlin = CalculateComplexPerlinNoise(uvw);
    rPerlin = smoothstep(0.1f, 0.9f, pow(rPerlin, 1.2f));

    // G: 粗いディテール（中くらいのモコモコ感）
    float gWorley = WorleyNoise3D_Seamless(uvw * 4.0f, 4.0f);

    // B: 細かいディテール（ちぎれ雲のようなシャープなエッジ削り用）
    float bWorley = WorleyNoise3D_Seamless(uvw * 8.0f, 8.0f);

    // A: 超細かいディテール（煙のような細密なザラつき、ミクロな空気感）
    float aWorley = WorleyNoise3D_Seamless(uvw * 16.0f, 16.0f);

    // 4つの独立したパーツとしてテクスチャに保存
    gOutputNoise[DTid] = float4(rPerlin, 1.0f - gWorley, 1.0f - bWorley, 1.0f - aWorley);
}