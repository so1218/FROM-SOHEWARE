#include "Quaternion.h" 
#include "MathUtils.h"      

// クォータニオンの積の定義
Quaternion Quaternion::operator*(const Quaternion& rhs) const
{
    return {
        y * rhs.z - z * rhs.y + x * rhs.w + w * rhs.x,
        z * rhs.x - x * rhs.z + y * rhs.w + w * rhs.y,
        x * rhs.y - y * rhs.x + z * rhs.w + w * rhs.z,
        w * rhs.w - x * rhs.x - y * rhs.y - z * rhs.z
    };
}

// 単位クォータニオンの定義
Quaternion Quaternion::Identity()
{
    return Quaternion(0, 0, 0, 1);
}

Quaternion Quaternion::MakeFromTwoVectors(const Vector3& from, const Vector3& to)
{
    Vector3 v0 = from.Normalize(); // 入力ベクトルを正規化
    Vector3 v1 = to.Normalize();   // ターゲットベクトルを正規化

    float dot = v0.Dot(v1); // 内積を計算

    // ほとんど同じ方向を向いている場合、Identityクォータニオンを返す
    // 浮動小数点誤差を考慮して少し余裕を持たせる
    const float kDotThreshold = 0.9999f;
    if (dot > kDotThreshold)
    {
        return Quaternion::Identity(); // 回転不要
    }

    // ほとんど反対方向を向いている場合 (180度回転が必要な場合)
    if (dot < -kDotThreshold)
    {
        // 任意の直交する軸を見つけて180度回転させる
        // まず、v0と直交するベクトルを試す (X軸が優先)
        Vector3 axis = Vector3(1.0f, 0.0f, 0.0f).Cross(v0);
        // もしX軸とv0が平行に近い場合 (外積が小さい場合)、Y軸を試す
        if (axis.LengthSq() < 0.0001f) // LengthSq() で計算負荷を軽減
        {
            axis = Vector3(0.0f, 1.0f, 0.0f).Cross(v0);
        }
        axis = axis.Normalize(); // 回転軸を正規化
        return Quaternion::FromAxisAngle(axis, Math::PI); // 180度回転
    }

    // 通常のケース: 軸と角度からクォータニオンを生成
    Vector3 axis = v0.Cross(v1).Normalize(); // 回転軸は2つのベクトルの外積
    float angle = std::acos(dot);           // 回転角度は内積からアークコサインで計算

    return Quaternion::FromAxisAngle(axis, angle);
}

Quaternion Quaternion::FromMatrix(const Matrix4x4& m) {
    Quaternion q;
    float trace = m.m[0][0] + m.m[1][1] + m.m[2][2]; // 対角成分の合計

    if (trace > 0.0f) {
        float s = std::sqrt(trace + 1.0f) * 2.0f;
        q.w = 0.25f * s;
        q.x = (m.m[2][1] - m.m[1][2]) / s;
        q.y = (m.m[0][2] - m.m[2][0]) / s;
        q.z = (m.m[1][0] - m.m[0][1]) / s;
    }
    else if ((m.m[0][0] > m.m[1][1]) && (m.m[0][0] > m.m[2][2])) {
        float s = std::sqrt(1.0f + m.m[0][0] - m.m[1][1] - m.m[2][2]) * 2.0f;
        q.w = (m.m[2][1] - m.m[1][2]) / s;
        q.x = 0.25f * s;
        q.y = (m.m[0][1] + m.m[1][0]) / s;
        q.z = (m.m[0][2] + m.m[2][0]) / s;
    }
    else if (m.m[1][1] > m.m[2][2]) {
        float s = std::sqrt(1.0f + m.m[1][1] - m.m[0][0] - m.m[2][2]) * 2.0f;
        q.w = (m.m[0][2] - m.m[2][0]) / s;
        q.x = (m.m[0][1] + m.m[1][0]) / s;
        q.y = 0.25f * s;
        q.z = (m.m[1][2] + m.m[2][1]) / s;
    }
    else {
        float s = std::sqrt(1.0f + m.m[2][2] - m.m[0][0] - m.m[1][1]) * 2.0f;
        q.w = (m.m[1][0] - m.m[0][1]) / s;
        q.x = (m.m[0][2] + m.m[2][0]) / s;
        q.y = (m.m[1][2] + m.m[2][1]) / s;
        q.z = 0.25f * s;
    }

    return q;
}

Vector3 Quaternion::QuaternionToEuler(const Quaternion& q)
{
    Vector3 euler;

    // Y軸（ピッチ）
    float sinp = 2.0f * (q.w * q.y - q.z * q.x);
    if (std::abs(sinp) >= 1)
        euler.y = std::copysign(Math::PI / 2, sinp); // 90度クランプ
    else
        sinp = std::clamp(sinp, -1.0f, 1.0f);
        euler.y = std::asin(sinp);

    // X軸（ロール）
    float sinr = 2.0f * (q.w * q.x + q.y * q.z);
    float cosr = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
    euler.x = std::atan2(sinr, cosr);

    // Z軸（ヨー）
    float siny = 2.0f * (q.w * q.z + q.x * q.y);
    float cosy = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
    euler.z = std::atan2(siny, cosy);

    return euler;
}

// ノルムの定義
float Quaternion::Norm() const
{
    return std::sqrt(x * x + y * y + z * z + w * w);
}

// 正規化の定義
Quaternion Quaternion::Normalize() const
{
    float n = Norm();
    
    if (n == 0.0f) return Identity();
    return Quaternion(x / n, y / n, z / n, w / n);
}

// 共役の定義
Quaternion Quaternion::Conjugate() const
{
    return Quaternion(-x, -y, -z, w);
}

// 逆クォータニオンの定義
Quaternion Quaternion::Inverse() const
{
    float n = Norm();
    // 非常に小さいノルムの場合、Identityを返す
    if (n < 1e-6f) return Identity(); // ゼロに近い値は0とみなす
    float invSq = 1.0f / (n * n);
    return Quaternion(-x * invSq, -y * invSq, -z * invSq, w * invSq);
}

// ベクトルを回転の定義
Vector3 Quaternion::RotateVector(const Vector3& v) const
{
    Quaternion p(v.x, v.y, v.z, 0.0f);
    // this->Conjugate() は呼び出し元の共役を返す
    Quaternion result = (*this) * p * this->Conjugate();
    return { result.x, result.y, result.z };
}

// 軸と角度からクォータニオンを作成の定義
Quaternion Quaternion::FromAxisAngle(const Vector3& axis, float angle)
{
    Vector3 nAxis = axis.Normalize();
    float half = angle * 0.5f;
    float s = sinf(half);
    return Quaternion(nAxis.x * s, nAxis.y * s, nAxis.z * s, cosf(half));
}

// 球面線形補間の定義
Quaternion Quaternion::Slerp(const Quaternion& q0, const Quaternion& q1, float t)
{
    // 内積で角度を取得
    float cosTheta = Dot(q0, q1);
    Quaternion q1mod = q1;

    // 最短経路補間のため符号反転
    if (cosTheta < 0.0f)
    {
        q1mod = Quaternion(-q1.x, -q1.y, -q1.z, -q1.w);
        cosTheta = -cosTheta;
    }

    // 角度が小さい場合は線形補間にフォールバック
    const float THRESHOLD = 0.9995f;
    if (cosTheta > THRESHOLD)
    {
        Quaternion result(q0.x + t * (q1mod.x - q0.x),
            q0.y + t * (q1mod.y - q0.y),
            q0.z + t * (q1mod.z - q0.z),
            q0.w + t * (q1mod.w - q0.w));
        return result.Normalize();
    }

    // 通常の球面線形補間
    float theta = acosf(cosTheta);
    float sinTheta = sinf(theta);

    // sinThetaがほぼ0なら線形補間にフォールバック
    if (std::abs(sinTheta) < 1e-6f)
    {
        Quaternion result(q0.x * (1.0f - t) + q1mod.x * t,
            q0.y * (1.0f - t) + q1mod.y * t,
            q0.z * (1.0f - t) + q1mod.z * t,
            q0.w * (1.0f - t) + q1mod.w * t);
        return result.Normalize();
    }

    // 球面補間の重みを計算
    float w0 = sinf((1.0f - t) * theta) / sinTheta;
    float w1 = sinf(t * theta) / sinTheta;

    // 補間結果を返す
    return Quaternion(q0.x * w0 + q1mod.x * w1,
        q0.y * w0 + q1mod.y * w1,
        q0.z * w0 + q1mod.z * w1,
        q0.w * w0 + q1mod.w * w1);
}

// ドット積の定義
float Quaternion::Dot(const Quaternion& q1, const Quaternion& q2)
{
    return q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w;
}

Quaternion Quaternion::QuaternionFromEuler(const Vector3& euler)
{
    Quaternion qx = Quaternion::FromAxisAngle({ 1,0,0 }, euler.x);
    Quaternion qy = Quaternion::FromAxisAngle({ 0,1,0 }, euler.y);
    Quaternion qz = Quaternion::FromAxisAngle({ 0,0,1 }, euler.z);

    // 順序は用途により変わるが、一般的にはZYX順
    return qz * qy * qx;
}

Quaternion Quaternion::LookRotation(const Vector3& forward, const Vector3& up)
{
    Vector3 f = forward.Normalize();
    Vector3 u = up.Normalize();

    // f と u から右方向ベクトルを計算
    Vector3 r = u.Cross(f).Normalize();

    // 修正された上方向ベクトルを計算
    Vector3 newUp = f.Cross(r);

    // 回転行列を作成
    Matrix4x4 mat;
    mat.m[0][0] = r.x;    mat.m[1][0] = r.y;    mat.m[2][0] = r.z;    mat.m[3][0] = 0.0f;
    mat.m[0][1] = newUp.x; mat.m[1][1] = newUp.y; mat.m[2][1] = newUp.z; mat.m[3][1] = 0.0f;
    mat.m[0][2] = f.x;    mat.m[1][2] = f.y;    mat.m[2][2] = f.z;    mat.m[3][2] = 0.0f;
    mat.m[0][3] = 0.0f;   mat.m[1][3] = 0.0f;   mat.m[2][3] = 0.0f;   mat.m[3][3] = 1.0f;

    // 行列からクォータニオンを作成する関数が必要
    return Quaternion::FromMatrix(mat);
}

Matrix4x4 Quaternion::ToMatrix() const {
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

    // 1行目
    result.m[0][0] = 1.0f - 2.0f * (yy + zz);
    result.m[0][1] = 2.0f * (xy + wz);
    result.m[0][2] = 2.0f * (xz - wy);
    result.m[0][3] = 0.0f;

    // 2行目
    result.m[1][0] = 2.0f * (xy - wz);
    result.m[1][1] = 1.0f - 2.0f * (xx + zz);
    result.m[1][2] = 2.0f * (yz + wx);
    result.m[1][3] = 0.0f;

    // 3行目
    result.m[2][0] = 2.0f * (xz + wy);
    result.m[2][1] = 2.0f * (yz - wx);
    result.m[2][2] = 1.0f - 2.0f * (xx + yy);
    result.m[2][3] = 0.0f;

    // 4行目
    result.m[3][0] = 0.0f;
    result.m[3][1] = 0.0f;
    result.m[3][2] = 0.0f;
    result.m[3][3] = 1.0f;

    return result;
}