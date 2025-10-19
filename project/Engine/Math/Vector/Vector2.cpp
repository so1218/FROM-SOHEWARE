#include "Vector2.h"
#include <cmath>

// デフォルトコンストラクタ
Vector2::Vector2() : x(0), y(0) {}

// 引数付きコンストラクタ
Vector2::Vector2(float x, float y) : x(x), y(y) {}

// 代入演算子オーバーロード
Vector2& Vector2::operator+=(const Vector2& other) {
    x += other.x; y += other.y; return *this;
}

Vector2& Vector2::operator-=(const Vector2& other) {
    x -= other.x; y -= other.y; return *this;
}

Vector2& Vector2::operator*=(float scalar) {
    x *= scalar; y *= scalar; return *this;
}

Vector2& Vector2::operator/=(float scalar) {
    if (scalar != 0) { x /= scalar; y /= scalar; }
    else { x = y = 0; } // ゼロ除算の場合は全要素を0に
    return *this;
}

// 二項演算子オーバーロード (constが付いているため、元のオブジェクトは変更しない)
Vector2 Vector2::operator+(const Vector2& other) const {
    return Vector2(x + other.x, y + other.y);
}

Vector2 Vector2::operator-(const Vector2& other) const {
    return Vector2(x - other.x, y - other.y);
}

Vector2 Vector2::operator*(float scalar) const {
    return Vector2(x * scalar, y * scalar);
}

Vector2 Vector2::operator/(float scalar) const {
    return scalar != 0 ? Vector2(x / scalar, y / scalar) : Vector2(0, 0);
}

// 比較演算子オーバーロード
bool Vector2::operator==(const Vector2& other) const {
    return x == other.x && y == other.y;
}

bool Vector2::operator!=(const Vector2& other) const {
    return !(*this == other);
}

// その他のメンバ関数
float Vector2::Length() const {
    return std::sqrtf(x * x + y * y);
}

float Vector2::LengthSq() const
{
    return x * x + y * y;
}


Vector2 Vector2::Normalize() const {
    float len = Length();
    return len != 0 ? Vector2(x / len, y / len) : Vector2(0, 0);
}

float Vector2::Dot(const Vector2& other) const {
    return x * other.x + y * other.y;
}

// 静的メソッド
float Vector2::DistanceSquared(const Vector2& v1, const Vector2& v2) {
    float dx = v1.x - v2.x;
    float dy = v1.y - v2.y;
    return dx * dx + dy * dy;
}

float Vector2::Distance(const Vector2& v1, const Vector2& v2) {
    return std::sqrt(DistanceSquared(v1, v2));
}

// 二項演算子オーバーロード (constが付いているため、元のオブジェクトは変更しない)
Vector2Int Vector2Int::operator+(const Vector2Int& other) const {
    return Vector2Int(x + other.x, y + other.y);
}
