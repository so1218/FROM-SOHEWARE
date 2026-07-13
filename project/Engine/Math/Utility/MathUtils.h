#pragma once
#include "Matrix.h"
#include "Vector.h"

namespace FE
{

class WorldTransform;

namespace Math
{

// 定数
constexpr float PI = 3.14159265358979323846f;

// ワールド座標をビューポート座標へ変換
Vector3 Project(const Vector3 worldPosition,
    float viewportX, float viewportY,
    float viewportWidth, float viewportHeight,
    const Matrix4x4 viewProjection);

// 色変換
Vector4 Uint32ToColorVector(uint32_t color);
uint32_t ColorVectorToUint32(const Vector4& color);

// 指定範囲の乱数生成
float RandomFloat(float min, float max);
int RandomInt(int min, int max);

// 度数法から弧度法への変換
float ToRadians(float degrees);

// 外積
Vector3 CrossProduct(const Vector3& v1, const Vector3& v2);

// ワールド座標をスクリーン座標へ変換
Vector2 WorldToScreen(const Vector3& worldPos, const Matrix4x4& viewProjection, float screenWidth, float screenHeight);

// 最小値を返す
template<typename T>
T MyMin(const T& a, const T& b)
{
    return (a < b) ? a : b;
}

// 最大値を返す
template<typename T>
T MyMax(const T& a, const T& b)
{
    return (a > b) ? a : b;
}

// 値を範囲内に制限
template<typename T>
T Clamp(const T& value, const T& minVal, const T& maxVal)
{
    return MyMax(minVal, MyMin(value, maxVal));
}

// 線形補間
template <typename T>
inline T Lerp(const T& a, const T& b, float t)
{
    return a * (1.0f - t) + b * t;
}

// 簡易的な擬似乱数ノイズ
inline float Fade(float t) { return t * t * t * (t * (t * 6 - 15) + 10); }
inline float NoiseLerp(float t, float a, float b) { return a + t * (b - a); }
inline float Grad(int hash, float x, float y, float z) {
    int h = hash & 15;
    float u = h < 8 ? x : y;
    float v = h < 4 ? y : h == 12 || h == 14 ? x : z;
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}


inline float PerlinNoise(float x, float y, float z)
{
    static const int p[512] = { 
        151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
        190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,88,237,149,56,87,174,
        20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,
        230,220,105,92,41,55,46,245,40,244,102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,
        18,169,200,196,135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,5,202,
        38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,223,183,170,213,119,248,152,
        2,44,154,163, 70,221,153,101,155,167, 43,172,9,129,22,39,253, 19,98,108,110,79,113,224,232,178,185,
        112,104,218,246,97,228,251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
        49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,138,236,205,93,222,114,
        67,29,24,72,243,141,128,195,78,66,215,61,156,180,
    };
    auto simpleHash = [](int x, int y, int z) {
        return (x * 73856093 ^ y * 19349663 ^ z * 83492791) % 512;
        };

    int X = (int)floor(x) & 255;
    int Y = (int)floor(y) & 255;
    int Z = (int)floor(z) & 255;
    x -= floor(x);
    y -= floor(y);
    z -= floor(z);

    float u = Fade(x);
    float v = Fade(y);
    float w = Fade(z);

    int A = simpleHash(X, Y, Z);
    int B = simpleHash(X + 1, Y, Z);
    int AA = simpleHash(X, Y + 1, Z);
    int BA = simpleHash(X + 1, Y + 1, Z);
    int AB = simpleHash(X, Y, Z + 1);
    int BB = simpleHash(X + 1, Y, Z + 1);
    int AAA = simpleHash(X, Y + 1, Z + 1);
    int BAA = simpleHash(X + 1, Y + 1, Z + 1);

    return NoiseLerp(w, NoiseLerp(v, NoiseLerp(u, Grad(A, x, y, z), Grad(B, x - 1, y, z)),
        NoiseLerp(u, Grad(AA, x, y - 1, z), Grad(BA, x - 1, y - 1, z))),
        NoiseLerp(v, NoiseLerp(u, Grad(AB, x, y, z - 1), Grad(BB, x - 1, y, z - 1)),
            NoiseLerp(u, Grad(AAA, x, y - 1, z - 1), Grad(BAA, x - 1, y - 1, z - 1))));
}

}
}