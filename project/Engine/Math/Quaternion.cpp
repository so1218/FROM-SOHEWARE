#include "Quaternion.h"
#include "MathUtils.h"
#include <cmath>
#include <algorithm>
#include <limits>

// 微小値定数（ゼロ除算防止用）
static const float kEpsilon = 1e-6f;

// クォータニオンの積
Quaternion Quaternion::operator*(const Quaternion& rhs) const
{
    return {
        y * rhs.z - z * rhs.y + x * rhs.w + w * rhs.x,
        z * rhs.x - x * rhs.z + y * rhs.w + w * rhs.y,
        x * rhs.y - y * rhs.x + z * rhs.w + w * rhs.z,
        w * rhs.w - x * rhs.x - y * rhs.y - z * rhs.z
    };
}

// 単位クォータニオン
Quaternion Quaternion::Identity()
{
    return Quaternion(0.0f, 0.0f, 0.0f, 1.0f);
}

// 2つのベクトル間の回転を作成
Quaternion Quaternion::MakeFromTwoVectors(const Vector3& from, const Vector3& to)
{
    // 入力ベクトルの長さをチェックして正規化
    float fromLenSq = from.LengthSq();
    float toLenSq = to.LengthSq();

    // どちらかがゼロベクトルなら回転不能なので単位元を返す
    if (fromLenSq < kEpsilon || toLenSq < kEpsilon)
    {
        return Quaternion::Identity();
    }

    Vector3 v0 = from.Normalize();
    Vector3 v1 = to.Normalize();

    float dot = v0.Dot(v1);

    // 完全に同じ方向 (1.0)
    if (dot >= 1.0f - kEpsilon)
    {
        return Quaternion::Identity();
    }

    // 完全に反対方向 (-1.0)
    if (dot <= -1.0f + kEpsilon)
    {
        // 180度回転。回転軸はv0に垂直な任意のベクトルが必要
        Vector3 axis = Vector3(1.0f, 0.0f, 0.0f).Cross(v0);

        // もしX軸と平行で外積が0に近いなら、Y軸を試す
        if (axis.LengthSq() < kEpsilon)
        {
            axis = Vector3(0.0f, 1.0f, 0.0f).Cross(v0);
        }

        axis = axis.Normalize();
        return Quaternion::FromAxisAngle(axis, Math::PI);
    }

    // 通常ケース
    // 外積で回転軸を計算
    Vector3 cross = v0.Cross(v1);
    float crossLen = cross.Length();

    // 外積が極端に小さい（平行に近い）場合の保護
    if (crossLen < kEpsilon)
    {
        return Quaternion::Identity();
    }

    Vector3 axis = cross / crossLen; // 手動正規化（再計算回避）
    float angle = std::acos(std::clamp(dot, -1.0f, 1.0f)); // acosの入力域外エラー防止

    return Quaternion::FromAxisAngle(axis, angle);
}

// 行列からクォータニオン
Quaternion Quaternion::FromMatrix(const Matrix4x4& m)
{
    Quaternion q;
    float trace = m.m[0][0] + m.m[1][1] + m.m[2][2];

    if (trace > 0.0f) {
        float s = std::sqrt(trace + 1.0f) * 2.0f;
        // ゼロ除算保護
        if (s < kEpsilon) s = 1.0f;
        float invS = 1.0f / s;

        q.w = 0.25f * s;
        q.x = (m.m[2][1] - m.m[1][2]) * invS;
        q.y = (m.m[0][2] - m.m[2][0]) * invS;
        q.z = (m.m[1][0] - m.m[0][1]) * invS;
    }
    else if ((m.m[0][0] > m.m[1][1]) && (m.m[0][0] > m.m[2][2])) {
        float s = std::sqrt(1.0f + m.m[0][0] - m.m[1][1] - m.m[2][2]) * 2.0f;
        if (s < kEpsilon) s = 1.0f;
        float invS = 1.0f / s;

        q.w = (m.m[2][1] - m.m[1][2]) * invS;
        q.x = 0.25f * s;
        q.y = (m.m[0][1] + m.m[1][0]) * invS;
        q.z = (m.m[0][2] + m.m[2][0]) * invS;
    }
    else if (m.m[1][1] > m.m[2][2]) {
        float s = std::sqrt(1.0f + m.m[1][1] - m.m[0][0] - m.m[2][2]) * 2.0f;
        if (s < kEpsilon) s = 1.0f;
        float invS = 1.0f / s;

        q.w = (m.m[0][2] - m.m[2][0]) * invS;
        q.x = (m.m[0][1] + m.m[1][0]) * invS;
        q.y = 0.25f * s;
        q.z = (m.m[1][2] + m.m[2][1]) * invS;
    }
    else {
        float s = std::sqrt(1.0f + m.m[2][2] - m.m[0][0] - m.m[1][1]) * 2.0f;
        if (s < kEpsilon) s = 1.0f;
        float invS = 1.0f / s;

        q.w = (m.m[1][0] - m.m[0][1]) * invS;
        q.x = (m.m[0][2] + m.m[2][0]) * invS;
        q.y = (m.m[1][2] + m.m[2][1]) * invS;
        q.z = 0.25f * s;
    }

    return q;
}

// クォータニオンからオイラー角
Vector3 Quaternion::QuaternionToEuler(const Quaternion& q)
{
    // 計算精度向上のため、処理前に正規化したコピーを使う
    Quaternion nq = q.Normalize();
    Vector3 euler;

    // Y軸（ピッチ）
    float sinp = 2.0f * (nq.w * nq.y - nq.z * nq.x);
    if (std::abs(sinp) >= 1.0f)
        euler.y = std::copysign(Math::PI / 2.0f, sinp); // 90度クランプ
    else
        euler.y = std::asin(sinp);

    // X軸（ロール）
    float sinr = 2.0f * (nq.w * nq.x + nq.y * nq.z);
    float cosr = 1.0f - 2.0f * (nq.x * nq.x + nq.y * nq.y);
    euler.x = std::atan2(sinr, cosr);

    // Z軸（ヨー）
    float siny = 2.0f * (nq.w * nq.z + nq.x * nq.y);
    float cosy = 1.0f - 2.0f * (nq.y * nq.y + nq.z * nq.z);
    euler.z = std::atan2(siny, cosy);

    return euler;
}

float Quaternion::Norm() const
{
    return std::sqrt(x * x + y * y + z * z + w * w);
}

// 正規化
Quaternion Quaternion::Normalize() const
{
    float n = Norm();
    // ゼロ除算防止
    if (n < kEpsilon) return Identity();

    float invN = 1.0f / n;
    return Quaternion(x * invN, y * invN, z * invN, w * invN);
}

Quaternion Quaternion::Conjugate() const
{
    return Quaternion(-x, -y, -z, w);
}

// 逆クォータニオン
Quaternion Quaternion::Inverse() const
{
    float nSq = x * x + y * y + z * z + w * w; // Norm()の二乗
    if (nSq < kEpsilon) return Identity(); // ゼロ除算防止

    float invSq = 1.0f / nSq;
    return Quaternion(-x * invSq, -y * invSq, -z * invSq, w * invSq);
}

Vector3 Quaternion::RotateVector(const Vector3& v) const
{
    Quaternion p(v.x, v.y, v.z, 0.0f);

    Quaternion inv = this->Conjugate();
    Quaternion result = (*this) * p * inv;

    return { result.x, result.y, result.z };
}

// 軸と角度から作成
Quaternion Quaternion::FromAxisAngle(const Vector3& axis, float angle)
{
    // 軸の長さチェック
    float axisLenSq = axis.LengthSq();
    if (axisLenSq < kEpsilon)
    {
        // 軸が無効なら回転なし
        return Identity();
    }

    Vector3 nAxis = axis.Normalize(); 
    float half = angle * 0.5f;
    float s = sinf(half);
    return Quaternion(nAxis.x * s, nAxis.y * s, nAxis.z * s, cosf(half));
}

// 球面線形補間
Quaternion Quaternion::Slerp(const Quaternion& q0, const Quaternion& q1, float t)
{
    float cosTheta = Dot(q0, q1);
    Quaternion q1mod = q1;

    if (cosTheta < 0.0f)
    {
        q1mod = Quaternion(-q1.x, -q1.y, -q1.z, -q1.w);
        cosTheta = -cosTheta;
    }

    // 角度が非常に小さい場合は線形補間
    // 閾値を少し広げて安全性を高める
    if (cosTheta >= 1.0f - kEpsilon)
    {
        return q0;
    }

    float theta = std::acos(std::clamp(cosTheta, -1.0f, 1.0f));
    float sinTheta = std::sin(theta);

    // ゼロ除算防止
    if (std::abs(sinTheta) < kEpsilon)
    {
        // 線形補間して正規化
        Quaternion result(
            q0.x * (1.0f - t) + q1mod.x * t,
            q0.y * (1.0f - t) + q1mod.y * t,
            q0.z * (1.0f - t) + q1mod.z * t,
            q0.w * (1.0f - t) + q1mod.w * t
        );
        return result.Normalize();
    }

    float w0 = std::sin((1.0f - t) * theta) / sinTheta;
    float w1 = std::sin(t * theta) / sinTheta;

    return Quaternion(
        q0.x * w0 + q1mod.x * w1,
        q0.y * w0 + q1mod.y * w1,
        q0.z * w0 + q1mod.z * w1,
        q0.w * w0 + q1mod.w * w1
    );
}

float Quaternion::Dot(const Quaternion& q1, const Quaternion& q2)
{
    return q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w;
}

Quaternion Quaternion::QuaternionFromEuler(const Vector3& euler)
{
    // FromAxisAngleが安全になったので、そのまま使用可能
    Quaternion qx = Quaternion::FromAxisAngle({ 1,0,0 }, euler.x);
    Quaternion qy = Quaternion::FromAxisAngle({ 0,1,0 }, euler.y);
    Quaternion qz = Quaternion::FromAxisAngle({ 0,0,1 }, euler.z);

    // ZYX順
    return qz * qy * qx;
}

// LookRotation
Quaternion Quaternion::LookRotation(const Vector3& forward, const Vector3& up)
{
    // 前方ベクトルの長さチェック
    float fLenSq = forward.LengthSq();
    if (fLenSq < kEpsilon)
    {
        return Quaternion::Identity(); // 向きがない場合は回転なし
    }

    Vector3 f = forward.Normalize();

    // アップベクトルのチェック
    Vector3 u = up;
    if (u.LengthSq() < kEpsilon)
    {
        u = Vector3(0.0f, 1.0f, 0.0f); // デフォルトUp
    }
    else
    {
        u = u.Normalize();
    }

    // 外積で右ベクトルを計算
    Vector3 r = u.Cross(f);
    float rLenSq = r.LengthSq();

    // 正規化でゼロ除算が発生
    if (rLenSq < kEpsilon)
    {
        // 平行な場合の回避策
        if (std::abs(u.y) > 0.99f) 
        {
            u = Vector3(0.0f, 0.0f, 1.0f);
        }
        else 
        {
            u = Vector3(0.0f, 1.0f, 0.0f);
        }

        // 再計算
        r = u.Cross(f);
        // それでもゼロならdentity
        if (r.LengthSq() < kEpsilon) return Quaternion::Identity();
    }

    r = r.Normalize();

    // 正しいUpベクトルを再計算 (直交化)
    Vector3 newUp = f.Cross(r); // 正規化された同士の直交ベクトルなので正規化済み

    // 行列成分の設定
    Matrix4x4 mat;
    mat.m[0][0] = r.x;     mat.m[1][0] = r.y;     mat.m[2][0] = r.z;     mat.m[3][0] = 0.0f;
    mat.m[0][1] = newUp.x; mat.m[1][1] = newUp.y; mat.m[2][1] = newUp.z; mat.m[3][1] = 0.0f;
    mat.m[0][2] = f.x;     mat.m[1][2] = f.y;     mat.m[2][2] = f.z;     mat.m[3][2] = 0.0f;
    mat.m[0][3] = 0.0f;    mat.m[1][3] = 0.0f;    mat.m[2][3] = 0.0f;    mat.m[3][3] = 1.0f;

    return Quaternion::FromMatrix(mat);
}

Matrix4x4 Quaternion::ToMatrix() const 
{
    Matrix4x4 result;

    float xx = x * x;
    float yy = y * y;
    float zz = z * z;
    float xy = x * y;
    float xz = x * z;
    float yz = y * z;
    float wx = w * x;
    float wy = w * y;
    float wz = w * z;

    result.m[0][0] = 1.0f - 2.0f * (yy + zz);
    result.m[0][1] = 2.0f * (xy + wz);
    result.m[0][2] = 2.0f * (xz - wy);
    result.m[0][3] = 0.0f;

    result.m[1][0] = 2.0f * (xy - wz);
    result.m[1][1] = 1.0f - 2.0f * (xx + zz);
    result.m[1][2] = 2.0f * (yz + wx);
    result.m[1][3] = 0.0f;

    result.m[2][0] = 2.0f * (xz + wy);
    result.m[2][1] = 2.0f * (yz - wx);
    result.m[2][2] = 1.0f - 2.0f * (xx + yy);
    result.m[2][3] = 0.0f;

    result.m[3][0] = 0.0f;
    result.m[3][1] = 0.0f;
    result.m[3][2] = 0.0f;
    result.m[3][3] = 1.0f;

    return result;
}