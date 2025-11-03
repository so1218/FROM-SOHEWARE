#pragma once

#include "Matrix.h"
#include "Vector.h"

#include <algorithm>

class WorldTransform;

namespace Math
{
    // 定数定義
    constexpr float PI = 3.14159265358979323846f;
    constexpr float TWO_PI = PI * 2.0f;
    constexpr float HALF_PI = PI * 0.5f;

    // --- 基本関数 ---

    // 画面座標への変換
    Vector3 Project(const Vector3 worldPosition, float viewportX, float viewportY, float viewportWidth, float viewportHeight, const Matrix4x4 viewProjection);

    // 色変換
    Vector4 Uint32ToColorVector(uint32_t color);
    uint32_t ColorVectorToUint32(const Vector4& color);

    // 乱数
    float RandomFloat(float min, float max);

    // 度からラジアンへ
    float ToRadians(float degrees);

    // --- テンプレート関数 ---

    template<typename T>
    T MyMin(const T& a, const T& b) {
        return (a < b) ? a : b;
    }

    template<typename T>
    T MyMax(const T& a, const T& b) {
        return (a > b) ? a : b;
    }

    // Clamp (標準ライブラリの std::clamp を使うのがベストですが、自作するなら)
    template<typename T>
    T Clamp(const T& value, const T& minVal, const T& maxVal) 
    {
        return MyMax(minVal, MyMin(value, maxVal));
    }

    // --- ベクトル演算 ---

    // 外積
    inline Vector3 CrossProduct(const Vector3& v1, const Vector3& v2)
    {
        return
        {
            v1.y * v2.z - v1.z * v2.y,
            v1.z * v2.x - v1.x * v2.z,
            v1.x * v2.y - v1.y * v2.x
        };
    }

    // --- 補間 (Lerp: Linear Interpolation) ---

    // float の線形補間
    inline float Lerp(float a, float b, float t)
    {
        return a + (b - a) * t;
    }

    // Vector3 の線形補間
    // (Vector3 に operator+ と operator* が実装されている前提)
    inline Vector3 Lerp(const Vector3& a, const Vector3& b, float t)
    {
        // return { a.x + ... } よりも、こちらの方が一貫性があります
        return a * (1.0f - t) + b * t;
    }

    // Vector4 の線形補間 (ご提示のコードのまま)
    inline Vector4 Lerp(const Vector4& start, const Vector4& end, float t) {
        return start * (1.0f - t) + end * t;
    }

    inline uint32_t LerpColor(uint32_t startColor, uint32_t endColor, float t)
    {
        // 1. Vector4 に分解
        Vector4 startVec = Uint32ToColorVector(startColor);
        Vector4 endVec = Uint32ToColorVector(endColor);

        Vector4 resultVec = Lerp(startVec, endVec, t);

        return Math::ColorVectorToUint32(resultVec);
    }

} 
