#pragma once
#include <cmath>

class Matrix4x4;

class Vector4
{
public:
    float x, y, z, w;

    // コンストラクタ
    Vector4();
    Vector4(float x, float y, float z, float w);

    // 代入演算子オーバーロード
    Vector4& operator+=(const Vector4& other);
    Vector4& operator-=(const Vector4& other);
    Vector4& operator*=(float scalar);
    Vector4& operator/=(float scalar);

    // 二項演算子オーバーロード
    Vector4 operator+(const Vector4& other) const;
    Vector4 operator-(const Vector4& other) const;
    Vector4 operator*(float scalar) const;
    Vector4 operator/(float scalar) const;

    // 比較演算子オーバーロード
    bool operator==(const Vector4& other) const;
    bool operator!=(const Vector4& other) const;

    // その他のメンバ関数
    float Length() const;
    Vector4 Normalized() const;
    float Dot(const Vector4& other) const;
};

struct Vector4Int
{
    int x, y, z, w;
};

Vector4 operator*(const Matrix4x4& mat, const Vector4& vec);