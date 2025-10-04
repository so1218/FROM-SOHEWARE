#pragma once
#include <cmath>
#include "Vector.h"
#include "Matrix.h"

class Quaternion
{
public:
    float x, y, z, w;

    // コンストラクタ
    Quaternion() : x(0), y(0), z(0), w(1) {}
    Quaternion(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

    // 演算子オーバーロードの宣言
    Quaternion operator*(const Quaternion& rhs) const;

    // 静的メンバ関数の宣言
    static Quaternion Identity();
    static Quaternion FromAxisAngle(const Vector3& axis, float angle);
    static Quaternion Slerp(const Quaternion& q0, const Quaternion& q1, float t);
    static float Dot(const Quaternion& q1, const Quaternion& q2);
    static Quaternion QuaternionFromEuler(const Vector3& euler);
    static Quaternion LookRotation(const Vector3& forward, const Vector3& up);
    static Quaternion MakeFromTwoVectors(const Vector3& from, const Vector3& to);
    static Quaternion FromMatrix(const Matrix4x4& m);
    static Vector3 QuaternionToEuler(const Quaternion& q);

    // メンバ関数の宣言
    float Norm() const;
    Quaternion Normalize() const;
    Quaternion Conjugate() const;
    Quaternion Inverse() const;
    Vector3 RotateVector(const Vector3& v) const;
    Matrix4x4 ToMatrix() const;
};