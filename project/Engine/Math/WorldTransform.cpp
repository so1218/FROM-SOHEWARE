#include "pch.h"
#include "WorldTransform.h"

namespace FS
{

// デフォルトコンストラクタ
WorldTransform::WorldTransform()
    : scale_({ 1, 1, 1 }), translation_({ 0, 0, 0 }), rotation_(Quaternion::Identity())
{
    UpdateMatrix(); // コンストラクタで初期行列を生成
}

// コンストラクタ(オイラー角を使用)
WorldTransform::WorldTransform(const Vector3& scale, const Vector3& rotation, const Vector3& translation)
    : scale_(scale), translation_(translation), rotation_(Quaternion::QuaternionFromEuler(rotation))
{
    UpdateMatrix();
}

// コンストラクタ(クォータニオンを使用)
WorldTransform::WorldTransform(const Vector3& scale, const Quaternion& rotation, const Vector3& translation)
    : scale_(scale), translation_(translation), rotation_(rotation)
{
    UpdateMatrix();
}

WorldTransform::WorldTransform(const WorldTransform& other)
    : scale_(other.scale_), translation_(other.translation_), rotation_(other.rotation_),
    matWorld_(other.matWorld_), matWorldPrev_(other.matWorldPrev_)
{
}

WorldTransform& WorldTransform::operator=(const WorldTransform& other)
{
    if (this != &other)
    {
        scale_ = other.scale_;
        translation_ = other.translation_;
        rotation_ = other.rotation_;
        matWorld_ = other.matWorld_;
        matWorldPrev_ = other.matWorldPrev_;
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
    Initialize(scale, Quaternion::QuaternionFromEuler(rotation), translation);
}

// 初期化関数(クォータニオンを使用)
void WorldTransform::Initialize(
    const Vector3& scale,
    const Quaternion& rotation,
    const Vector3& translation)
{
    parent_ = nullptr;
    scale_ = scale;
    rotation_ = rotation;
    translation_ = translation;
    UpdateMatrix();
}

// 行列を更新する関数
void WorldTransform::UpdateMatrix() 
{
    // ローカル行列を計算
    Matrix4x4 localMat = Matrix4x4::MakeAffine(
        scale_,
        rotation_,
        translation_
    );

    if (parent_ != nullptr)
    {
        // 親が設定されている場合、親のワールド行列を乗算
        matWorld_ = localMat * parent_->matWorld_;
    }
    else 
    {
        // 親がいない場合は、ローカル行列がワールド行列
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
    translation_ = translation;
    rotation_ = Quaternion::QuaternionFromEuler(rotation);
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
        Matrix4x4::Decompose(localMat, scale_, rotation_, translation_);
    }
    else
    {
        // 親なしならローカル=ワールド
        Matrix4x4::Decompose(matWorld_, scale_, rotation_, translation_);
    }
}

void WorldTransform::ApplyWorldMatrix()
{
    // ワールド行列からローカル情報（スケール・回転・移動）を取り出す
    Matrix4x4 world = matWorld_;
    Matrix4x4::Decompose(world, scale_, rotation_, translation_);

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
    rotation_ = rotation;
    UpdateMatrix();
}

}
