#pragma once

class Vector3
{
public:
    float x, y, z;

    // コンストラクタ
    Vector3() : x(0), y(0), z(0) {}
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

    // 複合代入演算子の宣言
    Vector3& operator+=(const Vector3& other);
    Vector3& operator-=(const Vector3& other);
    Vector3& operator*=(float scalar);
    Vector3& operator/=(float scalar);

    // 二項演算子の宣言
    Vector3 operator+(const Vector3& other) const;
    Vector3 operator-(const Vector3& other) const;
    Vector3 operator*(float scalar) const;
    Vector3 operator/(float scalar) const;
    friend Vector3 operator*(float scalar, const Vector3& vec);

    // 単項マイナス演算子
    Vector3 operator-() const;

    // 比較演算子の宣言
    bool operator==(const Vector3& other) const;
    bool operator!=(const Vector3& other) const;

    // メンバ関数の宣言
    float Length() const;
    float LengthSq() const;
    Vector3 Normalize() const;
    float Dot(const Vector3& other) const;
    Vector3 Cross(const Vector3& other) const;

    // 静的メンバ関数の宣言
    static float DistanceSquared(const Vector3& v1, const Vector3& v2);
    static Vector3 Slerp(const Vector3& start, const Vector3& end, float t);
    static Vector3 Lerp(const Vector3& current, const Vector3& target, float maxDelta);
    static Vector3 CatmullRomInterpolation(const std::vector<Vector3>& controlPoints, float t);

    // 方向ベクトルを返す静的関数
    static Vector3 Up() { return Vector3(0.0f, 1.0f, 0.0f); }
    static Vector3 Down() { return Vector3(0.0f, -1.0f, 0.0f); }
    static Vector3 Right() { return Vector3(1.0f, 0.0f, 0.0f); }
    static Vector3 Left() { return Vector3(-1.0f, 0.0f, 0.0f); }
    static Vector3 Forward() { return Vector3(0.0f, 0.0f, 1.0f); }
    static Vector3 Back() { return Vector3(0.0f, 0.0f, -1.0f); }
    static Vector3 Zero() { return Vector3(0.0f, 0.0f, 0.0f); }
   
};

Vector3 operator*(float scalar, const Vector3& vec);

struct Vector3Int
{
    int x, y, z;
};

