#pragma once

#include <cmath>

#include "Vector.h"

class WorldTransform;
class Quaternion;

class Matrix4x4
{
public:
    float m[4][4];

    // コンストラクタ
    Matrix4x4();
    Matrix4x4(float values[4][4]);

    // 演算子オーバーロード
    Matrix4x4 operator+(const Matrix4x4& other) const;
    Matrix4x4 operator*(const Matrix4x4& other) const;

    // メンバ関数
    Matrix4x4 Transpose() const;
    Vector3 Transform(const Vector3& vec) const;
    Vector3 TransformNormal(const Vector3& v) const;
    Vector3 TransformPoint(const Vector3& point) const;
    Vector3 TransformVector(const Vector3& v) const;
  
    // 静的メンバ関数
    static Matrix4x4 MakeIdentity();
    static Matrix4x4 MakeTranslate(const Vector3& translate);
    static Matrix4x4 MakeScale(const Vector3& scale);
    static Matrix4x4 MakeRotateX(float radian);
    static Matrix4x4 MakeRotateY(float radian);
    static Matrix4x4 MakeRotateZ(float radian);
    static Matrix4x4 MakeRotateXYZ(const Vector3& rotate);
    static Matrix4x4 MakeAffine(const Vector3& scale, const Vector3& rotate, const Vector3& translate);
    static Matrix4x4 MakeAffine(const Vector3& scale, const Quaternion& rotation, const Vector3& translation);
    static Matrix4x4 MakeOrthographic(float left, float top, float right, float bottom, float nearClip, float farClip);
    static Matrix4x4 MakeOrthographic(float width, float height, float nearClip, float farClip);
    static Matrix4x4 MakePerspectiveFov(float fovY, float aspectRatio, float nearClip, float farClip);
    static Matrix4x4 MakeViewport(float left, float top, float width, float height, float minDepth, float maxDepth);
    static Matrix4x4 MakeLookAt(const Vector3& eye, const Vector3& target, const Vector3& up);
    static Matrix4x4 Inverse(const Matrix4x4& m);
    static Matrix4x4 MakeWVPMatrix2D(const WorldTransform& worldTransform, float width, float height);  
    static void Decompose(
        const Matrix4x4& matrix,
        Vector3& outScale,
        Quaternion& outRotation,
        Vector3& outTranslation);
    static void ExtractTranslationAndRotation(
        const Matrix4x4& matrix,
        Vector3& outTranslation,
        Quaternion& outRotation
    );
    static Matrix4x4 RemoveScale(const Matrix4x4& mat);
};

