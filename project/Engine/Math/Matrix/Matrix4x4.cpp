#include "Matrix4x4.h" 
#include "WorldTransform.h" 

// コンストラクタ(初期化なし)
Matrix4x4::Matrix4x4() {
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            m[i][j] = 0.0f;
        }
    }
}

// コンストラクタ(値を指定して初期化)
Matrix4x4::Matrix4x4(float values[4][4]) {
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            m[i][j] = values[i][j];
        }
    }
}

// 行列の加算
Matrix4x4 Matrix4x4::operator+(const Matrix4x4& other) const {
    Matrix4x4 result;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            result.m[i][j] = m[i][j] + other.m[i][j];
        }
    }
    return result;
}

// 行列の乗算
Matrix4x4 Matrix4x4::operator*(const Matrix4x4& other) const {
    Matrix4x4 result;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            result.m[i][j] = 0;
            for (int k = 0; k < 4; ++k) {
                result.m[i][j] += m[i][k] * other.m[k][j];
            }
        }
    }
    return result;
}

// 行列の転置
Matrix4x4 Matrix4x4::Transpose() const {
    Matrix4x4 result;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            result.m[j][i] = m[i][j];
        }
    }
    return result;
}

// 単位行列 (静的メンバ関数)
Matrix4x4 Matrix4x4::MakeIdentity() {
    Matrix4x4 matrix = {}; // ゼロ初期化
    matrix.m[0][0] = 1.0f;
    matrix.m[1][1] = 1.0f;
    matrix.m[2][2] = 1.0f;
    matrix.m[3][3] = 1.0f;
    return matrix;
}

// 平行移動行列 (静的メンバ関数)
Matrix4x4 Matrix4x4::MakeTranslate(const Vector3& translate) {
    Matrix4x4 matrix = MakeIdentity(); // 単位行列で初期化
    matrix.m[3][0] = translate.x;
    matrix.m[3][1] = translate.y;
    matrix.m[3][2] = translate.z;
    return matrix;
}

// 拡大縮小行列 (静的メンバ関数)
Matrix4x4 Matrix4x4::MakeScale(const Vector3& scale) {
    Matrix4x4 matrix = MakeIdentity();
    matrix.m[0][0] = scale.x;
    matrix.m[1][1] = scale.y;
    matrix.m[2][2] = scale.z;
    return matrix;
}

// X軸回転行列 (静的メンバ関数)
Matrix4x4 Matrix4x4::MakeRotateX(float radian) {
    Matrix4x4 matrix = MakeIdentity();
    float cosTheta = std::cos(radian);
    float sinTheta = std::sin(radian);
    matrix.m[1][1] = cosTheta;
    matrix.m[1][2] = sinTheta;
    matrix.m[2][1] = -sinTheta;
    matrix.m[2][2] = cosTheta;
    return matrix;
}

// Y軸回転行列 (静的メンバ関数)
Matrix4x4 Matrix4x4::MakeRotateY(float radian) {
    Matrix4x4 matrix = MakeIdentity();
    float cosTheta = std::cos(radian);
    float sinTheta = std::sin(radian);
    matrix.m[0][0] = cosTheta;
    matrix.m[0][2] = -sinTheta;
    matrix.m[2][0] = sinTheta;
    matrix.m[2][2] = cosTheta;
    return matrix;
}

// Z軸回転行列 (静的メンバ関数)
Matrix4x4 Matrix4x4::MakeRotateZ(float radian) {
    Matrix4x4 matrix = MakeIdentity();
    float cosTheta = std::cos(radian);
    float sinTheta = std::sin(radian);
    matrix.m[0][0] = cosTheta;
    matrix.m[0][1] = sinTheta;
    matrix.m[1][0] = -sinTheta;
    matrix.m[1][1] = cosTheta;
    return matrix;
}

// XYZ軸回転行列 (静的メンバ関数)
Matrix4x4 Matrix4x4::MakeRotateXYZ(const Vector3& rotate) {
    Matrix4x4 rotateX = MakeRotateX(rotate.x);
    Matrix4x4 rotateY = MakeRotateY(rotate.y);
    Matrix4x4 rotateZ = MakeRotateZ(rotate.z);
    return rotateX * rotateY * rotateZ; // 例: ZXY順
}

// アフィン変換行列
Matrix4x4 Matrix4x4::MakeAffine(const Vector3& scale, const Vector3& rotate, const Vector3& translate) {
    Matrix4x4 scaleM = MakeScale(scale);
    Matrix4x4 rotateM = MakeRotateXYZ(rotate);
    Matrix4x4 translateM = MakeTranslate(translate);
    return scaleM * rotateM * translateM;
}

// クォータニオンを使ったアフィン変換行列の生成
Matrix4x4 Matrix4x4::MakeAffine(const Vector3& scale, const Quaternion& rotation, const Vector3& translation) {
    // 回転行列を生成
    Matrix4x4 rotMat = rotation.ToMatrix();
    // スケール行列
    Matrix4x4 scaleMat = Matrix4x4::MakeScale(scale);
    // 並進行列
    Matrix4x4 transMat = Matrix4x4::MakeTranslate(translation);
    // アフィン行列の構成
    return scaleMat * rotMat * transMat;
}

// 正射影行列
Matrix4x4 Matrix4x4::MakeOrthographic(float left, float top, float right, float bottom, float nearClip, float farClip) {
    Matrix4x4 result = {}; // ゼロ初期化
    result.m[0][0] = 2.0f / (right - left);
    result.m[1][1] = 2.0f / (top - bottom);
    result.m[2][2] = 1.0f / (farClip - nearClip);
    result.m[3][0] = (left + right) / (left - right);
    result.m[3][1] = (top + bottom) / (bottom - top);
    result.m[3][2] = -nearClip / (farClip - nearClip);
    result.m[3][3] = 1.0f;
    return result;
}

Matrix4x4 Matrix4x4::MakeOrthographic(float width, float height, float nearClip, float farClip)
{
    // 中心(0,0)を基準に、左右・上下に幅の半分ずつ広げる
    float halfWidth = width * 0.5f;
    float halfHeight = height * 0.5f;

    return MakeOrthographic(-halfWidth, halfHeight, halfWidth, -halfHeight, nearClip, farClip);
}

// 透視投影行列
Matrix4x4 Matrix4x4::MakePerspectiveFov(float fovY, float aspectRatio, float nearClip, float farClip) {
    Matrix4x4 matrix = {};
    float f = 1.0f / std::tanf(fovY * 0.5f);
    matrix.m[0][0] = f / aspectRatio;
    matrix.m[1][1] = f;
    matrix.m[2][2] = farClip / (farClip - nearClip);
    matrix.m[2][3] = 1.0f;
    matrix.m[3][2] = (-farClip * nearClip) / (farClip - nearClip);
    return matrix;
}

// ビューポート変換行列
Matrix4x4 Matrix4x4::MakeViewport(float left, float top, float width, float height, float minDepth, float maxDepth) {
    Matrix4x4 result = {}; // ゼロ初期化
    result.m[0][0] = width / 2.0f;
    result.m[1][1] = -height / 2.0f;
    result.m[2][2] = maxDepth - minDepth;
    result.m[3][0] = left + width / 2.0f;
    result.m[3][1] = top + height / 2.0f;
    result.m[3][2] = minDepth;
    result.m[3][3] = 1.0f;
    return result;
}

// ルックアット行列
Matrix4x4 Matrix4x4::MakeLookAt(const Vector3& eye, const Vector3& target, const Vector3& up) {
    Vector3 zAxis = (target - eye).Normalize(); // Z軸 (ターゲット方向)
    Vector3 xAxis = up.Cross(zAxis).Normalize();      // X軸 (右方向)
    Vector3 yAxis = zAxis.Cross(xAxis);             // Y軸 (上方向)

    Matrix4x4 result;
    result.m[0][0] = xAxis.x;
    result.m[0][1] = yAxis.x;
    result.m[0][2] = zAxis.x;
    result.m[0][3] = 0.0f;

    result.m[1][0] = xAxis.y;
    result.m[1][1] = yAxis.y;
    result.m[1][2] = zAxis.y;
    result.m[1][3] = 0.0f;

    result.m[2][0] = xAxis.z;
    result.m[2][1] = yAxis.z;
    result.m[2][2] = zAxis.z;
    result.m[2][3] = 0.0f;

    result.m[3][0] = -xAxis.Dot(eye);
    result.m[3][1] = -yAxis.Dot(eye);
    result.m[3][2] = -zAxis.Dot(eye);
    result.m[3][3] = 1.0f;

    return result;
}

// 逆行列
Matrix4x4 Matrix4x4::Inverse(const Matrix4x4& m)
{
    Matrix4x4 result{};
    float inv[16], det;
    const float* src = &m.m[0][0];

    inv[0] = src[5] * src[10] * src[15] -
        src[5] * src[11] * src[14] -
        src[9] * src[6] * src[15] +
        src[9] * src[7] * src[14] +
        src[13] * src[6] * src[11] -
        src[13] * src[7] * src[10];

    inv[4] = -src[4] * src[10] * src[15] +
        src[4] * src[11] * src[14] +
        src[8] * src[6] * src[15] -
        src[8] * src[7] * src[14] -
        src[12] * src[6] * src[11] +
        src[12] * src[7] * src[10];

    inv[8] = src[4] * src[9] * src[15] -
        src[4] * src[11] * src[13] -
        src[8] * src[5] * src[15] +
        src[8] * src[7] * src[13] +
        src[12] * src[5] * src[11] -
        src[12] * src[7] * src[9];

    inv[12] = -src[4] * src[9] * src[14] +
        src[4] * src[10] * src[13] +
        src[8] * src[5] * src[14] -
        src[8] * src[6] * src[13] -
        src[12] * src[5] * src[10] +
        src[12] * src[6] * src[9];

    inv[1] = -src[1] * src[10] * src[15] +
        src[1] * src[11] * src[14] +
        src[9] * src[2] * src[15] -
        src[9] * src[3] * src[14] -
        src[13] * src[2] * src[11] +
        src[13] * src[3] * src[10];

    inv[5] = src[0] * src[10] * src[15] -
        src[0] * src[11] * src[14] -
        src[8] * src[2] * src[15] +
        src[8] * src[3] * src[14] +
        src[12] * src[2] * src[11] -
        src[12] * src[3] * src[10];

    inv[9] = -src[0] * src[9] * src[15] +
        src[0] * src[11] * src[13] +
        src[8] * src[1] * src[15] -
        src[8] * src[3] * src[13] -
        src[12] * src[1] * src[11] +
        src[12] * src[3] * src[9];

    inv[13] = src[0] * src[9] * src[14] -
        src[0] * src[10] * src[13] -
        src[8] * src[1] * src[14] +
        src[8] * src[2] * src[13] +
        src[12] * src[1] * src[10] -
        src[12] * src[2] * src[9];

    inv[2] = src[1] * src[6] * src[15] -
        src[1] * src[7] * src[14] -
        src[5] * src[2] * src[15] +
        src[5] * src[3] * src[14] +
        src[13] * src[2] * src[7] -
        src[13] * src[3] * src[6];

    inv[6] = -src[0] * src[6] * src[15] +
        src[0] * src[7] * src[14] +
        src[4] * src[2] * src[15] -
        src[4] * src[3] * src[14] -
        src[12] * src[2] * src[7] +
        src[12] * src[3] * src[6];

    inv[10] = src[0] * src[5] * src[15] -
        src[0] * src[7] * src[13] -
        src[4] * src[1] * src[15] +
        src[4] * src[3] * src[13] +
        src[12] * src[1] * src[7] -
        src[12] * src[3] * src[5];

    inv[14] = -src[0] * src[5] * src[14] +
        src[0] * src[6] * src[13] +
        src[4] * src[1] * src[14] -
        src[4] * src[2] * src[13] -
        src[12] * src[1] * src[6] +
        src[12] * src[2] * src[5];

    inv[3] = -src[1] * src[6] * src[11] +
        src[1] * src[7] * src[10] +
        src[5] * src[2] * src[11] -
        src[5] * src[3] * src[10] -
        src[9] * src[2] * src[7] +
        src[9] * src[3] * src[6];

    inv[7] = src[0] * src[6] * src[11] -
        src[0] * src[7] * src[10] -
        src[4] * src[2] * src[11] +
        src[4] * src[3] * src[10] +
        src[8] * src[2] * src[7] -
        src[8] * src[3] * src[6];

    inv[11] = -src[0] * src[5] * src[11] +
        src[0] * src[7] * src[9] +
        src[4] * src[1] * src[11] -
        src[4] * src[3] * src[9] -
        src[8] * src[1] * src[7] +
        src[8] * src[3] * src[5];

    inv[15] = src[0] * src[5] * src[10] -
        src[0] * src[6] * src[9] -
        src[4] * src[1] * src[10] +
        src[4] * src[2] * src[9] +
        src[8] * src[1] * src[6] -
        src[8] * src[2] * src[5];

    det = src[0] * inv[0] + src[1] * inv[4] + src[2] * inv[8] + src[3] * inv[12];

    det = 1.0f / det;

    Matrix4x4 resultMatrix;
    for (int i = 0; i < 16; ++i)
    {
        ((float*)resultMatrix.m)[i] = inv[i] * det;
    }

    return resultMatrix;
}

Matrix4x4 Matrix4x4::MakeWVPMatrix2D(const WorldTransform& worldTransform, float width, float height)
{
    Matrix4x4 worldMatrix = Matrix4x4::MakeAffine(worldTransform.scale_, worldTransform.rotation_, worldTransform.translation_);
    Matrix4x4 viewMatrix = Matrix4x4::MakeIdentity();
    Matrix4x4 projectionMatrix = Matrix4x4::MakeOrthographic(0.0f, 0.0f, float(width), float(height), 0.0f, 100.0f);
    return worldMatrix * (viewMatrix * projectionMatrix);
}

// ベクトルの変換 (メンバ関数)
Vector3 Matrix4x4::Transform(const Vector3& vec) const {
    Vector3 result;
    result.x = vec.x * m[0][0] + vec.y * m[1][0] + vec.z * m[2][0] + 1.0f * m[3][0];
    result.y = vec.x * m[0][1] + vec.y * m[1][1] + vec.z * m[2][1] + 1.0f * m[3][1];
    result.z = vec.x * m[0][2] + vec.y * m[1][2] + vec.z * m[2][2] + 1.0f * m[3][2];
    float w = vec.x * m[0][3] + vec.y * m[1][3] + vec.z * m[2][3] + 1.0f * m[3][3];
    // 透視変換の場合、wで除算
    if (w != 0.0f) {
        result.x /= w;
        result.y /= w;
        result.z /= w;
    }
    return result;
}

Vector4 Matrix4x4::Transform(const Vector4& vec) const
{
    Vector4 result;
    // vec.w をそのまま使用して行列計算を行う
    result.x = vec.x * m[0][0] + vec.y * m[1][0] + vec.z * m[2][0] + vec.w * m[3][0];
    result.y = vec.x * m[0][1] + vec.y * m[1][1] + vec.z * m[2][1] + vec.w * m[3][1];
    result.z = vec.x * m[0][2] + vec.y * m[1][2] + vec.z * m[2][2] + vec.w * m[3][2];
    result.w = vec.x * m[0][3] + vec.y * m[1][3] + vec.z * m[2][3] + vec.w * m[3][3];

    // Vector4を返す変換では、通常ここで w 除算は行いません。
    // クリップ空間の座標や、射影変換の逆変換などで w の値そのものが必要になるためです。
    return result;
}

// 法線ベクトルの変換
Vector3 Matrix4x4::TransformNormal(const Vector3& v) const {
    Vector3 result;
    result.x = v.x * m[0][0] + v.y * m[1][0] + v.z * m[2][0];
    result.y = v.x * m[0][1] + v.y * m[1][1] + v.z * m[2][1];
    result.z = v.x * m[0][2] + v.y * m[1][2] + v.z * m[2][2];
    return result.Normalize(); // 変換後に正規化する
}

Vector3 Matrix4x4::TransformPoint(const Vector3& point) const
{
    // 4次元に拡張して行列と乗算する
    float x = point.x * m[0][0] + point.y * m[1][0] + point.z * m[2][0] + m[3][0];
    float y = point.x * m[0][1] + point.y * m[1][1] + point.z * m[2][1] + m[3][1];
    float z = point.x * m[0][2] + point.y * m[1][2] + point.z * m[2][2] + m[3][2];
    float w = point.x * m[0][3] + point.y * m[1][3] + point.z * m[2][3] + m[3][3];

    // wで除算（透視変換対応。w = 1 であれば何もしない）
    if (w != 0.0f && w != 1.0f) {
        x /= w;
        y /= w;
        z /= w;
    }

    return Vector3(x, y, z);
}

Vector3 Matrix4x4::TransformVector(const Vector3& v) const
{
    Vector3 result;
    result.x = m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z;
    result.y = m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z;
    result.z = m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z;
    return result;
}

void Matrix4x4::Decompose(
    const Matrix4x4& matrix,
    Vector3& outScale,
    Quaternion& outRotation,
    Vector3& outTranslation
) 
{
    // 平行移動（位置）
    outTranslation = Vector3(matrix.m[3][0], matrix.m[3][1], matrix.m[3][2]);

    // 回転 + スケールが混ざった軸ベクトル
    Vector3 colX(matrix.m[0][0], matrix.m[1][0], matrix.m[2][0]);
    Vector3 colY(matrix.m[0][1], matrix.m[1][1], matrix.m[2][1]);
    Vector3 colZ(matrix.m[0][2], matrix.m[1][2], matrix.m[2][2]);

    // スケールの抽出
    outScale.x = colX.Length();
    outScale.y = colY.Length();
    outScale.z = colZ.Length();

    // 回転だけを抽出（スケーリング除去）
    if (outScale.x != 0.0f) colX /= outScale.x;
    if (outScale.y != 0.0f) colY /= outScale.y;
    if (outScale.z != 0.0f) colZ /= outScale.z;

    // 回転行列の組み立て（列ベース）
    Matrix4x4 rotationMatrix = Matrix4x4::MakeIdentity();
    rotationMatrix.m[0][0] = colX.x; rotationMatrix.m[0][1] = colY.x; rotationMatrix.m[0][2] = colZ.x;
    rotationMatrix.m[1][0] = colX.y; rotationMatrix.m[1][1] = colY.y; rotationMatrix.m[1][2] = colZ.y;
    rotationMatrix.m[2][0] = colX.z; rotationMatrix.m[2][1] = colY.z; rotationMatrix.m[2][2] = colZ.z;

    // クォータニオンに変換
    outRotation = Quaternion::FromMatrix(rotationMatrix);
}

void Matrix4x4::ExtractTranslationAndRotation(
    const Matrix4x4& matrix,
    Vector3& outTranslation,
    Quaternion& outRotation
)
{
    // 位置は行列の4行目
    outTranslation = Vector3(matrix.m[3][0], matrix.m[3][1], matrix.m[3][2]);

    // 回転行列のみ抽出（スケーリングを前提に含まない）
    Matrix4x4 rotMat;
    rotMat.m[0][0] = matrix.m[0][0]; rotMat.m[0][1] = matrix.m[0][1]; rotMat.m[0][2] = matrix.m[0][2];
    rotMat.m[1][0] = matrix.m[1][0]; rotMat.m[1][1] = matrix.m[1][1]; rotMat.m[1][2] = matrix.m[1][2];
    rotMat.m[2][0] = matrix.m[2][0]; rotMat.m[2][1] = matrix.m[2][1]; rotMat.m[2][2] = matrix.m[2][2];

    // クォータニオンに変換
    outRotation = Quaternion::FromMatrix(rotMat);
}

Matrix4x4 Matrix4x4::RemoveScale(const Matrix4x4& mat)
{
    Matrix4x4 result = mat;

    // 各軸のスケーリング量を求める
    float sx = Vector3{ mat.m[0][0], mat.m[0][1], mat.m[0][2] }.Length();
    float sy = Vector3{ mat.m[1][0], mat.m[1][1], mat.m[1][2] }.Length();
    float sz = Vector3{ mat.m[2][0], mat.m[2][1], mat.m[2][2] }.Length();

    // スケーリングを除去（正規化）
    for (int i = 0; i < 3; ++i)
    {
        result.m[0][i] /= sx;
        result.m[1][i] /= sy;
        result.m[2][i] /= sz;
    }

    return result;
}
