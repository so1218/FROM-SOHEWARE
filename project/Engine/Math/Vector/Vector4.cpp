#include "Vector4.h"
#include "Matrix4x4.h"

// デフォルトコンストラクタ
Vector4::Vector4() : x(0), y(0), z(0), w(0) {}

// 引数付きコンストラクタ
Vector4::Vector4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

// 代入演算子オーバーロード
Vector4& Vector4::operator+=(const Vector4& other) 
{
    x += other.x; y += other.y; z += other.z; w += other.w; return *this;
}

Vector4& Vector4::operator-=(const Vector4& other) 
{
    x -= other.x; y -= other.y; z -= other.z; w -= other.w; return *this;
}

Vector4& Vector4::operator*=(float scalar) 
{
    x *= scalar; y *= scalar; z *= scalar; w *= scalar; return *this;
}

Vector4& Vector4::operator/=(float scalar) 
{
    if (scalar != 0) { x /= scalar; y /= scalar; z /= scalar; w /= scalar; }
    else { x = y = z = w = 0; } // ゼロ除算の場合は全要素を0に
    return *this;
}

// 二項演算子オーバーロード
Vector4 Vector4::operator+(const Vector4& other) const 
{
    return Vector4(x + other.x, y + other.y, z + other.z, w + other.w);
}

Vector4 Vector4::operator-(const Vector4& other) const 
{
    return Vector4(x - other.x, y - other.y, z - other.z, w - other.w);
}

Vector4 Vector4::operator*(float scalar) const 
{
    return Vector4(x * scalar, y * scalar, z * scalar, w * scalar);
}

Vector4 Vector4::operator/(float scalar) const 
{
    return scalar != 0 ? Vector4(x / scalar, y / scalar, z / scalar, w / scalar) : Vector4(0, 0, 0, 0);
}

// 比較演算子オーバーロード
bool Vector4::operator==(const Vector4& other) const
{
    return x == other.x && y == other.y && z == other.z && w == other.w;
}

bool Vector4::operator!=(const Vector4& other) const
{
    return !(*this == other);
}

// その他のメンバ関数
float Vector4::Length() const
{
    return std::sqrtf(x * x + y * y + z * z + w * w);
}

Vector4 Vector4::Normalized() const
{
    float len = Length();
    return len != 0 ? Vector4(x / len, y / len, z / len, w / len) : Vector4(0, 0, 0, 0);
}

float Vector4::Dot(const Vector4& other) const 
{
    return x * other.x + y * other.y + z * other.z + w * other.w;
}

Vector4 operator*(const Matrix4x4& mat, const Vector4& vec)
{
    Vector4 result;
    result.x = mat.m[0][0] * vec.x + mat.m[1][0] * vec.y + mat.m[2][0] * vec.z + mat.m[3][0] * vec.w;
    result.y = mat.m[0][1] * vec.x + mat.m[1][1] * vec.y + mat.m[2][1] * vec.z + mat.m[3][1] * vec.w;
    result.z = mat.m[0][2] * vec.x + mat.m[1][2] * vec.y + mat.m[2][2] * vec.z + mat.m[3][2] * vec.w;
    result.w = mat.m[0][3] * vec.x + mat.m[1][3] * vec.y + mat.m[2][3] * vec.z + mat.m[3][3] * vec.w;
    return result;
}