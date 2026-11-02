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

    // 親はコピーしない
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

    // 行列を更新する関数
    void UpdateMatrix();

    // 行列を更新する関数(オイラー角を使用)
    void UpdateMatrix(
        const Vector3& scale,
        const Vector3& rotation,
        const Vector3& translation);

    void DetachFromParent();
    void UpdateLocalFromWorld();

    void ApplyWorldMatrix();

    const Matrix4x4& GetWorldMatrix() const { return matWorld_; }
    // 行列を直接決める 
    void SetWorldMatrix(const Matrix4x4& worldMatrix) { matWorld_ = worldMatrix; }
    const Matrix4x4& GetPrevWorldMatrix() const { return matWorldPrev_; }
    Vector3 GetWorldPosition() const;

    // 特定のプロパティを更新するセッター
    void SetScale(const Vector3& scale);
    void SetTranslation(const Vector3& translation);
    void SetRotation(const Quaternion& rotation);

    // オイラー角。クォータニオンに変換して持つ
    void SetRotationEuler(const Vector3& euler) { SetRotation(Quaternion::QuaternionFromEuler(euler)); }
    Vector3 GetRotationEuler() const { return Quaternion::QuaternionToEuler(rotation_); }

private:
    // ワールド行列
    Matrix4x4 matWorld_;
    // 1フレーム前のワールド行列
    Matrix4x4 matWorldPrev_ = Matrix4x4::MakeIdentity();
    // 親のWorldTransformへのポインタ
    WorldTransform* parent_ = nullptr;
};

}
