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