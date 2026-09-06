#ifndef MATH_HLSLI
#define MATH_HLSLI

static const float PI = 3.1415926535f;
static const float kEpsilon = 1e-5f;

// -------------------------------------------------------------------------
// 汎用数学ヘルパー関数
// -------------------------------------------------------------------------

// 高速な5乗計算 (Schlick Approximation / SSS用)
float Pow5(float x)
{
    float x2 = x * x;
    return x2 * x2 * x;
}

float3 Pow5(float3 x)
{
    float3 x2 = x * x;
    return x2 * x2 * x;
}

// -------------------------------------------------------------------------
// 擬似乱数 / ハッシュ関数 (Dave_Hoskins 氏の Hash without Sine アルゴリズム)
// -------------------------------------------------------------------------

// 1Dスカラー -> 1Dスカラー [0.0, 1.0]
float Hash11(float p)
{
    p = frac(p * 0.1031f);
    p *= p + 33.33f;
    p *= p + p;
    return frac(p);
}

// 2D座標 -> 1Dスカラー [0.0, 1.0]
float Hash12(float2 p)
{
    float3 p3 = frac(float3(p.xyx) * 0.1031f);
    p3 += dot(p3, p3.yzx + 33.33f);
    return frac((p3.x + p3.y) * p3.z);
}

// 2D座標 -> 2Dベクトル [0.0, 1.0]
float2 Hash22(float2 p)
{
    float3 p3 = frac(float3(p.xyx) * float3(0.1031f, 0.1030f, 0.0973f));
    p3 += dot(p3, p3.yzx + 33.33f);
    return frac((p3.xx + p3.yz) * p3.zy);
}

// 1Dスカラー -> 3D方向ベクトル [-1.0, 1.0]
float3 Hash31(float p)
{
    float3 p3 = frac(float3(p, p, p) * float3(0.1031f, 0.1030f, 0.0973f));
    p3 += dot(p3, p3.yzx + 33.33f);
    return frac((p3.xxy + p3.yzz) * p3.zyx) * 2.0f - 1.0f;
}

// 3D座標 -> 3D方向ベクトル [-1.0, 1.0]
float3 Hash33(float3 p3)
{
    p3 = frac(p3 * float3(0.1031f, 0.1030f, 0.0973f));
    p3 += dot(p3, p3.yxz + 33.33f);
    return -1.0f + 2.0f * frac((p3.xxy + p3.yxx) * p3.zyx);
}

// スクリーン空間グラディエントノイズ (SSAO/SSR/ディザリング用) [0.0, 1.0]
float InterleavedGradientNoise(float2 pixelPos)
{
    float3 magic = float3(0.06711056f, 0.00583715f, 52.9829189f);
    return frac(magic.z * frac(dot(pixelPos, magic.xy)));
}

// -------------------------------------------------------------------------
// プロシージャルノイズ関数
// -------------------------------------------------------------------------

// 1D入力 (時間等) -> 3D空間の連続的で滑らかなノイズ [-1.0, 1.0]
float3 ValueNoise31(float p)
{
    float i = floor(p);
    float f = frac(p);
    
    // エルミート補間 (Smoothstep) による滑らかな繋ぎ合わせ
    f = f * f * (3.0f - 2.0f * f);
    return lerp(Hash31(i), Hash31(i + 1.0f), f);
}

#endif