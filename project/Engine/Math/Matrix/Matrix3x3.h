#pragma once
#include "Vector3.h"

namespace FE
{

class Matrix3x3
{
public:
    float m[3][3];

    // コンストラクタ：初期化なし
    Matrix3x3();

    // コンストラクタ：値を指定して初期化
    Matrix3x3(float values[3][3]);

    // 行列の加算
    Matrix3x3 operator+(const Matrix3x3& other) const;

    // 行列の乗算
    Matrix3x3 operator*(const Matrix3x3& other) const;

    // 行列の転置
    Matrix3x3 Transpose() const;

    Vector3 TransformVector(const Vector3& v) const;

    static Matrix3x3 FromBasis(const Vector3& right, const Vector3& up, const Vector3& forward);
 
};

}