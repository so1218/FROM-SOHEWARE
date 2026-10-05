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

}
}