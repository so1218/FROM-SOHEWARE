#pragma once

class Vector2
{
public:
    float x, y;

    // コンストラクタ
    Vector2();
    Vector2(float x, float y);

    // 代入演算子オーバーロード
    Vector2& operator+=(const Vector2& other);
    Vector2& operator-=(const Vector2& other);
    Vector2& operator*=(float scalar);
    Vector2& operator/=(float scalar);

    // 二項演算子オーバーロード
    Vector2 operator+(const Vector2& other) const;
    Vector2 operator-(const Vector2& other) const;
    Vector2 operator*(float scalar) const;
    Vector2 operator/(float scalar) const;

    // 比較演算子オーバーロード
    bool operator==(const Vector2& other) const;
    bool operator!=(const Vector2& other) const;

    // その他のメンバ関数
    float Length() const;
    float LengthSq() const;
    Vector2 Normalize() const;
    float Dot(const Vector2& other) const;

    // 静的メソッド
    static float DistanceSquared(const Vector2& v1, const Vector2& v2);
    static float Distance(const Vector2& v1, const Vector2& v2);
};

struct Vector2Int
{
    int x, y;

    // 二項演算子オーバーロード
    Vector2Int operator+(const Vector2Int& other) const;
};