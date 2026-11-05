#include "pch.h"
#include "WorldTransform.h"

namespace FS
{

namespace
{

bool IsSameVector(const Vector3& a, const Vector3& b)
{
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

bool IsSameQuaternion(const Quaternion& a, const Quaternion& b)
{
    return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
}

}

WorldTransform::WorldTransform()
    : scale_({ 1, 1, 1 }), translation_({ 0, 0, 0 }), rotation_(Quaternion::Identity())
{
    ComputeWorldMatrix();
}

// コンストラクタ(オイラー角を使用)
WorldTransform::WorldTransform(const Vector3& scale, const Vector3& rotation, const Vector3& translation)
    : scale_(scale), translation_(translation), rotation_(Quaternion::QuaternionFromEuler(rotation))
{
    ComputeWorldMatrix();
}

// コンストラクタ(クォータニオンを使用)
WorldTransform::WorldTransform(const Vector3& scale, const Quaternion& rotation, const Vector3& translation)
    : scale_(scale), translation_(translation), rotation_(rotation)
{
    ComputeWorldMatrix();
}

WorldTransform::WorldTransform(const WorldTransform& other)
{
    *this = other;
}

WorldTransform& WorldTransform::operator=(const WorldTransform& other)
{
    if (this != &other)
    {
        scale_ = other.scale_;
        translation_ = other.translation_;
        rotation_ = other.rotation_;
        matWorldPrev_ = other.matWorldPrev_;
        parent_ = nullptr;

        // 親の分も含んだ、コピーした時点のワールド行列を引き継ぐ
        matWorld_ = other.GetWorldMatrix();
        computedScale_ = scale_;
        computedTranslation_ = translation_;
        computedRotation_ = rotation_;
        computedParent_ = nullptr;
        computedParentVersion_ = 0;
        ++worldMatrixVersion_;
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
}

const Matrix4x4& WorldTransform::GetWorldMatrix() const
{
    RefreshWorldMatrix();
    return matWorld_;
}

void WorldTransform::SetWorldMatrix(const Matrix4x4& worldMatrix)
{
    matWorld_ = worldMatrix;
    computedScale_ = scale_;
    computedTranslation_ = translation_;
    computedRotation_ = rotation_;
    computedParent_ = parent_;
    computedParentVersion_ = parent_ ? parent_->GetWorldMatrixVersion() : 0;
    ++worldMatrixVersion_;
}

uint32_t WorldTransform::GetWorldMatrixVersion() const
{
    RefreshWorldMatrix();
    return worldMatrixVersion_;
}

void WorldTransform::RefreshWorldMatrix() const
{
    // 親を先に最新にする
    const uint32_t parentVersion = parent_ ? parent_->GetWorldMatrixVersion() : 0;

    const bool isUnchanged =
        IsSameVector(scale_, computedScale_) &&
        IsSameVector(translation_, computedTranslation_) &&
        IsSameQuaternion(rotation_, computedRotation_) &&
        parent_ == computedParent_ &&
        parentVersion == computedParentVersion_;
    if (isUnchanged)
    {
        return;
    }

    ComputeWorldMatrix();
}

void WorldTransform::ComputeWorldMatrix() const
{
    Matrix4x4 localMatrix = Matrix4x4::MakeAffine(scale_, rotation_, translation_);
    matWorld_ = parent_ ? localMatrix * parent_->GetWorldMatrix() : localMatrix;

    computedScale_ = scale_;
    computedTranslation_ = translation_;
    computedRotation_ = rotation_;
    computedParent_ = parent_;
    computedParentVersion_ = parent_ ? parent_->GetWorldMatrixVersion() : 0;
    ++worldMatrixVersion_;
}

void WorldTransform::DetachFromParent()
{
    // 今のワールド行列が保たれるように、親を外してからローカルを決め直す
    const Matrix4x4 worldMatrix = GetWorldMatrix();
    parent_ = nullptr;
    Matrix4x4::Decompose(worldMatrix, scale_, rotation_, translation_);
}

void WorldTransform::UpdateLocalFromWorld()
{
    const Matrix4x4 worldMatrix = GetWorldMatrix();
    const Matrix4x4 localMatrix = parent_ ? Matrix4x4::Inverse(parent_->GetWorldMatrix()) * worldMatrix : worldMatrix;
    Matrix4x4::Decompose(localMatrix, scale_, rotation_, translation_);
}

void WorldTransform::ApplyWorldMatrix()
{
    // 今のワールド行列をローカルにして、親から独立する
    DetachFromParent();
}

Vector3 WorldTransform::GetWorldPosition() const
{
    const Matrix4x4& worldMatrix = GetWorldMatrix();
    return { worldMatrix.m[3][0], worldMatrix.m[3][1], worldMatrix.m[3][2] };
}

}
