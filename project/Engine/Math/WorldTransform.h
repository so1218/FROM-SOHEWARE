#pragma once
#include "Vector.h"
#include "Matrix.h"
#include "Quaternion.h"

namespace FE
{

class WorldTransform
{
public:
    // ローカルスケール
    Vector3 scale_;
    // X,Y,Z軸回りのローカル回転角(オイラー角)
    Vector3 rotation_;
    // ローカル座標
    Vector3 translation_;
    // ワールド行列
    Matrix4x4 matWorld_;
    // 1フレーム前のワールド行列
    Matrix4x4 matWorldPrev_ = Matrix4x4::MakeIdentity();
    // 回転を保持するクォータニオン
    Quaternion rotationQuaternion_;

    // 親のWorldTransformへのポインタ
    WorldTransform* parent_;

public:
    // コンストラクタ
    WorldTransform();
    WorldTransform(const Vector3& scale, const Vector3& rotation, const Vector3& translation);
    WorldTransform(const Vector3& scale, const Quaternion& rotation, const Vector3& translation);

    // 明示的に定義する
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

    // 親を設定するメソッド
    void SetParent(WorldTransform* parent) { parent_ = parent; }

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
    Vector3 GetWorldPosition() const;

    // 特定のプロパティを更新するセッター
    void SetScale(const Vector3& scale);
    void SetTranslation(const Vector3& translation);
    void SetRotation(const Quaternion& rotation);

};

}