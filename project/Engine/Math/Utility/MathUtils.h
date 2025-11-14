#pragma once

#include "Matrix.h"
#include "Vector.h"

#include <algorithm>

class WorldTransform;

namespace Math
{
    // --- 定数定義 ---
    constexpr float PI = 3.14159265358979323846f;
    constexpr float TWO_PI = PI * 2.0f;
    constexpr float HALF_PI = PI * 0.5f;

    // --- 基本関数 ---

    // ワールド座標を画面座標に変換
    Vector3 Project(const Vector3 worldPosition,
        float viewportX, float viewportY,
        float viewportWidth, float viewportHeight,
        const Matrix4x4 viewProjection);

    // 色変換（32bitカラー <-> Vector4）
    Vector4 Uint32ToColorVector(uint32_t color);
    uint32_t ColorVectorToUint32(const Vector4& color);

    // 指定範囲のランダムな浮動小数を生成
    float RandomFloat(float min, float max);

    // 度をラジアンに変換
    float ToRadians(float degrees);

    // --- 汎用テンプレート関数 ---

    // 最小値を返す
    template<typename T>
    T MyMin(const T& a, const T& b) {
        return (a < b) ? a : b;
    }

    // 最大値を返す
    template<typename T>
    T MyMax(const T& a, const T& b) {
        return (a > b) ? a : b;
    }

    // 値を指定範囲 [minVal, maxVal] にクランプ（制限）
    template<typename T>
    T Clamp(const T& value, const T& minVal, const T& maxVal) {
        return MyMax(minVal, MyMin(value, maxVal));
    }

    // --- ベクトル演算 ---

    // 外積（右ねじ方向の法線ベクトルを返す）
    inline Vector3 CrossProduct(const Vector3& v1, const Vector3& v2)
    {
        return {
            v1.y * v2.z - v1.z * v2.y,
            v1.z * v2.x - v1.x * v2.z,
            v1.x * v2.y - v1.y * v2.x
        };
    }

    // --- 線形補間 (Lerp: Linear Interpolation) ---

    template <typename T>
    inline T Lerp(const T& a, const T& b, float t)
    {
        return a * (1.0f - t) + b * t;
    }
}