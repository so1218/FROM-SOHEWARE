#pragma once
#include "Vector.h"
#include "Matrix.h"
#include "Quaternion.h"

namespace FS
{

class WorldTransform
{
public:
    // ローカルスケール
    Vector3 scale_;
    // ローカル座標
    Vector3 translation_;
    // ローカル回転
    Quaternion rotation_;

public:
    WorldTransform();
    WorldTransform(const Vector3& scale, const Vector3& rotation, const Vector3& translation);
    WorldTransform(const Vector3& scale, const Quaternion& rotation, const Vector3& translation);

    // 親はコピーしない。ワールド行列はコピーした時点のものを引き継ぐ
    WorldTransform(const WorldTransform& other);
    WorldTransform& operator=(const WorldTransform& other);

    // 初期化関数(オイラー角を使用)
    void Initialize(
        const Vector3& scale = { 1, 1, 1 },
        const Vector3& rotation = { 0, 0, 0 },
        const Vector3& translation = { 0, 0, 0 });

    // 初期化関数(クォータニオンを使用)
    void Initialize(
        const Vector3& scale,
        const Quaternion& rotation,
        const Vector3& translation);

    void SetParent(WorldTransform* parent) { parent_ = parent; }
    WorldTransform* GetParent() const { return parent_; }

    void DetachFromParent();
    void UpdateLocalFromWorld();
    void ApplyWorldMatrix();

    const Matrix4x4& GetWorldMatrix() const;
    // 行列を直接決める。位置・回転・大きさか親が変わるまでは、この行列のまま
    void SetWorldMatrix(const Matrix4x4& worldMatrix);
    const Matrix4x4& GetPrevWorldMatrix() const { return matWorldPrev_; }
    Vector3 GetWorldPosition() const;

    void SetScale(const Vector3& scale) { scale_ = scale; }
    void SetTranslation(const Vector3& translation) { translation_ = translation; }
    void SetRotation(const Quaternion& rotation) { rotation_ = rotation; }

    // オイラー角。クォータニオンに変換して持つ
    void SetRotationEuler(const Vector3& euler) { SetRotation(Quaternion::QuaternionFromEuler(euler)); }
    Vector3 GetRotationEuler() const { return Quaternion::QuaternionToEuler(rotation_); }

private:
    // 前に計算したときから自分か親が変わっていたら、ワールド行列を計算し直す
    void RefreshWorldMatrix() const;
    void ComputeWorldMatrix() const;
    // 子が親が変わったかを知るための番号。ワールド行列が変わるたびに進む
    uint32_t GetWorldMatrixVersion() const;

    WorldTransform* parent_ = nullptr;

    // 1フレーム前のワールド行列
    Matrix4x4 matWorldPrev_ = Matrix4x4::MakeIdentity();

    // 最後に計算したときの値を覚えておき、変わったかを比べる
    mutable Matrix4x4 matWorld_;
    mutable Vector3 computedScale_;
    mutable Vector3 computedTranslation_;
    mutable Quaternion computedRotation_;
    mutable const WorldTransform* computedParent_ = nullptr;
    mutable uint32_t computedParentVersion_ = 0;
    mutable uint32_t worldMatrixVersion_ = 0;
};

}
