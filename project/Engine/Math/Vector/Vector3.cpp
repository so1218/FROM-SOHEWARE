#include "pch.h"
#include "Vector3.h"

namespace FE
{

// 複合代入演算子の定義
Vector3& Vector3::operator+=(const Vector3& other)
{
    x += other.x;
    y += other.y;
    z += other.z;
    return *this;
}

Vector3& Vector3::operator-=(const Vector3& other)
{
    x -= other.x;
    y -= other.y;
    z -= other.z;
    return *this;
}

Vector3& Vector3::operator*=(float scalar)
{
    x *= scalar;
    y *= scalar;
    z *= scalar;
    return *this;
}

Vector3& Vector3::operator/=(float scalar)
{
    // ゼロ除算を避けるための厳密な比較は、EPSILON値を使いたい
    if (scalar != 0.0f && std::fabs(scalar) > 1e-6f) // 小さすぎる値も0
    {
        x /= scalar;
        y /= scalar;
        z /= scalar;
    }
    else
    {
        x = y = z = 0; 
    }
    return *this;
}

// 二項演算子の定義

Vector3 Vector3::operator+(const Vector3& other) const
{
    return Vector3{ x + other.x, y + other.y, z + other.z };
}

Vector3 Vector3::operator-(const Vector3& other) const
{
    return Vector3(x - other.x, y - other.y, z - other.z);
}

Vector3 Vector3::operator*(float scalar) const
{
    return Vector3(x * scalar, y * scalar, z * scalar);
}

Vector3 operator*(float scalar, const Vector3& vec)
{
    return Vector3(vec.x * scalar, vec.y * scalar, vec.z * scalar);
}

Vector3 Vector3::operator/(float scalar) const
{
    return (scalar != 0.0f && std::fabs(scalar) > 1e-6f) ? Vector3(x / scalar, y / scalar, z / scalar) : Vector3(0, 0, 0);
}

// 単項マイナス演算子
Vector3 Vector3::operator-() const
{
    return Vector3(-x, -y, -z);
}

// 比較演算子の定義
bool Vector3::operator==(const Vector3& other) const
{
    return x == other.x && y == other.y && z == other.z;
}

bool Vector3::operator!=(const Vector3& other) const 
{
    return !(*this == other);
}

// その他のメンバ関数の定義
float Vector3::Length() const
{
    return std::sqrtf(x * x + y * y + z * z);
}

float Vector3::LengthSq() const
{
    return x * x + y * y + z * z;
}

Vector3 Vector3::Normalize() const
{
    float len = Length();
    if (len == 0.0f) // ゼロ除算防止
    {
        return Vector3(0, 0, 0);
    }
    return Vector3(x / len, y / len, z / len);
}

float Vector3::Dot(const Vector3& other) const
{
    return x * other.x + y * other.y + z * other.z;
}

Vector3 Vector3::Cross(const Vector3& other) const
{
    return Vector3(
        y * other.z - z * other.y,
        z * other.x - x * other.z,
        x * other.y - y * other.x
    );
}

// 静的メンバ関数の定義
float Vector3::DistanceSquared(const Vector3& v1, const Vector3& v2)
{
    float dx = v1.x - v2.x;
    float dy = v1.y - v2.y;
    float dz = v1.z - v2.z;
    return dx * dx + dy * dy + dz * dz;
}

Vector3 Vector3::Slerp(const Vector3& start, const Vector3& end, float t) {
    Vector3 startN = start.Normalize();
    Vector3 endN = end.Normalize();

    float dot = startN.Dot(endN);
    dot = std::clamp(dot, -1.0f, 1.0f); 
    float theta = std::acos(dot);

    const float EPSILON = 1e-6f;
    if (theta < EPSILON) 
    {
        return startN * (1.0f - t) + endN * t; // 通常の線形補間にフォールバック
    }

    float sinTheta = std::sin(theta);
    if (std::abs(sinTheta) < EPSILON)
    {
        return startN * (1.0f - t) + endN * t; // sin(theta)がほぼ0の場合
    }

    float s0 = std::sin((1.0f - t) * theta) / sinTheta;
    float s1 = std::sin(t * theta) / sinTheta;

    return startN * s0 + endN * s1;
}

Vector3 Vector3::Lerp(const Vector3& current, const Vector3& target, float maxDelta)
{
    Vector3 delta = target - current;
    float distanceSq = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z; 

    // maxDeltaよりも距離が小さいなら、目標地点に到達
    if (distanceSq <= maxDelta * maxDelta)
    { 
        return target;
    }

    // 距離に対して maxDelta 分だけ進めた位置を返す
    float distance = std::sqrt(distanceSq);
    float scale = maxDelta / distance;

    return current + delta * scale; 
}

Vector3 Vector3::CatmullRomInterpolation(const std::vector<Vector3>& controlPoints, float t_global)
{
    size_t count = controlPoints.size();

    if (count < 4) 
    {
        return Vector3();
    }

    size_t segmentCount = count - 3;

    float totalT = t_global * segmentCount;
    size_t segment = static_cast<size_t>(totalT);
    if (segment >= segmentCount)
    {
        segment = segmentCount - 1;
    }

    float t = totalT - segment;

    const Vector3& p0 = controlPoints[segment + 0];
    const Vector3& p1 = controlPoints[segment + 1];
    const Vector3& p2 = controlPoints[segment + 2];
    const Vector3& p3 = controlPoints[segment + 3];

    float t2 = t * t;
    float t3 = t2 * t;

    Vector3 result;

    result.x = 0.5f * ((2.0f * p1.x) + (-p0.x + p2.x) * t + (2.0f * p0.x - 5.0f * p1.x + 4.0f * p2.x - p3.x) * t2 + (-p0.x + 3.0f * p1.x - 3.0f * p2.x + p3.x) * t3);
    result.y = 0.5f * ((2.0f * p1.y) + (-p0.y + p2.y) * t + (2.0f * p0.y - 5.0f * p1.y + 4.0f * p2.y - p3.y) * t2 + (-p0.y + 3.0f * p1.y - 3.0f * p2.y + p3.y) * t3);
    result.z = 0.5f * ((2.0f * p1.z) + (-p0.z + p2.z) * t + (2.0f * p0.z - 5.0f * p1.z + 4.0f * p2.z - p3.z) * t2 + (-p0.z + 3.0f * p1.z - 3.0f * p2.z + p3.z) * t3);

    return result;
}

}