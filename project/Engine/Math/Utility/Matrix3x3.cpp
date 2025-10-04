#include "Matrix3x3.h"

// コンストラクタ(初期化なし)
Matrix3x3::Matrix3x3()
{
    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 3; ++j)
        {
            m[i][j] = 0.0f;
        }
    }
}

// コンストラクタ(値を指定して初期化)
Matrix3x3::Matrix3x3(float values[3][3])
{
    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 3; ++j)
        {
            m[i][j] = values[i][j];
        }
    }
}

// 行列の加算
Matrix3x3 Matrix3x3::operator+(const Matrix3x3& other) const
{
    Matrix3x3 result;
    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 3; ++j)
        {
            result.m[i][j] = m[i][j] + other.m[i][j];
        }
    }
    return result;
}

// 行列の乗算
Matrix3x3 Matrix3x3::operator*(const Matrix3x3& other) const
{
    Matrix3x3 result;
    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 3; ++j)
        {
            result.m[i][j] = 0; // 結果の要素を0に初期化
            for (int k = 0; k < 3; ++k)
            {
                result.m[i][j] += m[i][k] * other.m[k][j];
            }
        }
    }
    return result;
}

// 行列の転置
Matrix3x3 Matrix3x3::Transpose() const
{
    Matrix3x3 result;
    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 3; ++j)
        {
            result.m[j][i] = m[i][j]; // 行と列を入れ替える
        }
    }
    return result;
}

Vector3 Matrix3x3::TransformVector(const Vector3& v) const {
    return {
        m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z,
        m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z,
        m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z
    };
}

// 修正された FromBasis メソッド
Matrix3x3 Matrix3x3::FromBasis(const Vector3& right, const Vector3& up, const Vector3& forward)
{
   float values[3][3] = {
       {right.x, up.x, forward.x},
       {right.y, up.y, forward.y},
       {right.z, up.z, forward.z}
   };
   return Matrix3x3(values);
}