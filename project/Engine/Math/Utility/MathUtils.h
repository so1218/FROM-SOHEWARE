#pragma once

// 定数定義
constexpr float PI = 3.14159265358979323846f;
constexpr float TWO_PI = PI * 2.0f;
constexpr float HALF_PI = PI * 0.5f;

#include "Matrix.h"
#include "Vector.h"

#include <algorithm>

class WorldTransform;

Vector3 Project(const Vector3 worldPosition, float viewportX, float viewportY, float viewportWidth, float viewportHeight, const Matrix4x4 viewProjection);
Vector4 Uint32ToColorVector(uint32_t color);
uint32_t ColorVectorToUint32(const Vector4& color);
float RandomFloat(float min, float max);
float ToRadians(float degrees);
// テンプレート関数定義
template<typename T>
T MyMin(const T& a, const T& b) {
    return (a < b) ? a : b;
}

template<typename T>
T MyMax(const T& a, const T& b) {
    return (a > b) ? a : b;
}

template<typename T>
T MyClamp(const T& value, const T& minVal, const T& maxVal) {
    return MyMax(minVal, MyMin(value, maxVal));
}
// 2つの3Dベクトルの外積を計算する関数
inline Vector3 CrossProduct(const Vector3& v1, const Vector3& v2) {
    return {
        v1.y * v2.z - v1.z * v2.y,
        v1.z * v2.x - v1.x * v2.z,
        v1.x * v2.y - v1.y * v2.x
    };
}

inline Vector3 Lerp(const Vector3& a, const Vector3& b, float t)
{
    return {
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t
    };
}

inline Vector4 Lerp(const Vector4& start, const Vector4& end, float t) {
    return start * (1.0f - t) + end * t;
}
