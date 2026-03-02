#include "WorldTransform.h"

// デフォルトコンストラクタ
WorldTransform::WorldTransform()
    : scale_({ 1, 1, 1 }), rotation_({ 0, 0, 0 }), translation_({ 0, 0, 0 }),
    rotationQuaternion_(Quaternion::Identity()), parent_(nullptr) 
{
    UpdateMatrix(); // コンストラクタで初期行列を生成
}

// コンストラクタ(オイラー角を使用)
WorldTransform::WorldTransform(const Vector3& scale, const Vector3& rotation, const Vector3& translation)
    : scale_(scale), rotation_(rotation), translation_(translation),
    rotationQuaternion_(Quaternion::QuaternionFromEuler(rotation)), parent_(nullptr)
{
    UpdateMatrix();
}

// コンストラクタ(クォータニオンを使用)
WorldTransform::WorldTransform(const Vector3& scale, const Quaternion& rotation, const Vector3& translation)
    : scale_(scale), rotation_({ 0,0,0 }), translation_(translation),
    rotationQuaternion_(rotation), parent_(nullptr)
{
    UpdateMatrix();
}

WorldTransform::WorldTransform(const WorldTransform& other)
{
    scale_ = other.scale_;
    rotation_ = other.rotation_;
    translation_ = other.translation_;
    matWorld_ = other.matWorld_;
    rotationQuaternion_ = other.rotationQuaternion_;
    parent_ = nullptr; // コピーしない
}

WorldTransform& WorldTransform::operator=(const WorldTransform& other)
{
    if (this != &other)
    {
        scale_ = other.scale_;
        rotation_ = other.rotation_;
        translation_ = other.translation_;
        matWorld_ = other.matWorld_;
        rotationQuaternion_ = other.rotationQuaternion_;
        parent_ = nullptr; // コピーしない
    }
    return *this;
}

// 初期化関数(オイラー角を使用)
void WorldTransform::Initialize(
    const Vector3& scale,
    const Vector3& rotation,
    const Vector3& translation)
{
    // 単位行列に初期化
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            matWorld_.m[i][j] = (i == j) ? 1.0f : 0.0f;
    scale_ = scale;
    rotation_ = rotation;
    translation_ = translation;
    parent_ = nullptr;
    rotationQuaternion_ = Quaternion::QuaternionFromEuler(rotation_); // Euler角からクォータニオンを生成
    UpdateMatrix();
}

// 初期化関数(クォータニオンを使用)
void WorldTransform::Initialize(
    const Vector3& scale,
    const Quaternion& rotation,
    const Vector3& translation)
{
    // 単位行列に初期化
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            matWorld_.m[i][j] = (i == j) ? 1.0f : 0.0f;
    parent_ = nullptr;
    scale_ = { 1.0f, 1.0f, 1.0f };
    rotationQuaternion_ = Quaternion();
    translation_ = { 0.0f, 0.0f, 0.0f };
    UpdateMatrix();
}

// 行列を更新する関数
void WorldTransform::UpdateMatrix() 
{
    // ローカル行列を計算
    Matrix4x4 localMat = Matrix4x4::MakeAffine(
        scale_,
        rotationQuaternion_,
        translation_
    );

    if (parent_ != nullptr) {
        // 親が設定されている場合、親のワールド行列を乗算
        matWorld_ = localMat * parent_->matWorld_;
    }
    else {
        // 親がいない場合は、ローカル行列がワールド行列となる
        matWorld_ = localMat;
    }
}

// 全てのプロパティを更新して行列を更新する関数(オイラー角を使用)
void WorldTransform::UpdateMatrix(
    const Vector3& scale,
    const Vector3& rotation,
    const Vector3& translation)
{
    scale_ = scale;
    rotation_ = rotation;
    translation_ = translation;
    rotationQuaternion_ = Quaternion::QuaternionFromEuler(rotation_);
    UpdateMatrix(); 
}

void WorldTransform::DetachFromParent()
{
    // 親から離脱する前にワールド行列を更新
    UpdateMatrix();

    // 親を外す
    parent_ = nullptr;

    // ワールド行列を基にローカル変換を再計算
    UpdateLocalFromWorld();
}

void WorldTransform::UpdateLocalFromWorld()
{
    if (parent_)
    {
        // 親のワールド行列の逆行列を取得
        Matrix4x4 parentInv = Matrix4x4::Inverse(parent_->matWorld_);

        // 親の逆行列×自身のワールド行列でローカル行列を算出
        Matrix4x4 localMat = parentInv * matWorld_;

        // localMat からスケール・回転・平行移動を分解してセット
        Matrix4x4::Decompose(localMat, scale_, rotationQuaternion_, translation_);
    }
    else
    {
        // 親なしならローカル=ワールド
        Matrix4x4::Decompose(matWorld_, scale_, rotationQuaternion_, translation_);
    }
}

void WorldTransform::ApplyWorldMatrix()
{
    // ワールド行列からローカル情報（スケール・回転・移動）を取り出す
    Matrix4x4 world = matWorld_;

    // 行列を分解（スケール・回転・位置を抽出）
    Matrix4x4::Decompose(world, scale_, rotationQuaternion_, translation_);

    // 回転角（オイラー）も更新
    rotation_ = Quaternion::QuaternionToEuler(rotationQuaternion_);

    // 親を外して独立
    parent_ = nullptr;
}

Vector3 WorldTransform::GetWorldPosition() const
{
    return { matWorld_.m[3][0], matWorld_.m[3][1], matWorld_.m[3][2] };
}

// 特定のプロパティを更新するセッター
void WorldTransform::SetScale(const Vector3& scale) 
{
    scale_ = scale;
    UpdateMatrix();
}

void WorldTransform::SetTranslation(const Vector3& translation)
{
    translation_ = translation;
    UpdateMatrix();
}

void WorldTransform::SetRotation(const Quaternion& rotation) 
{
    rotationQuaternion_ = rotation;
    UpdateMatrix();
}