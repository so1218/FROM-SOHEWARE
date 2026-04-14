#include "pch.h"
#include "Collider.h"
#include "GameObject.h"
#include "DebugDraw.h"
#include "CollisionManager.h"
#include "GameObjectManager.h"

namespace FE
{ 

Collider::Collider(GameObject* owner) : owner_(owner)
{
    // 親が存在し、かつ親がマネージャーを知っていれば自動登録
    if (owner_ && owner_->GetManager())
    {
        manager_ = owner_->GetManager()->GetCollisionManager();
        if (manager_)
        {
            manager_->AddCollider(this);
        }
    }
}

Collider::~Collider()
{
    if (manager_)
    {
        manager_->RemoveCollider(this);
    }
}

Vector3 Collider::GetWorldPosition() const
{
    // 親が存在しない場合は原点を返す（安全対策）
    if (!owner_) return centerOffset_;

    // 親のTransformを取得
    const WorldTransform& transform = owner_->GetTransform();

    // スケールの適用
    Vector3 scaledOffset =
    {
        centerOffset_.x * transform.scale_.x,
        centerOffset_.y * transform.scale_.y,
        centerOffset_.z * transform.scale_.z
    };

    // クォータニオンを使ってオフセットベクトルを回転
    Vector3 rotatedOffset = transform.rotationQuaternion_.RotateVector(scaledOffset);

    // 親のワールド座標に、回転・スケール済みのオフセットを足す
    return transform.GetWorldPosition() + rotatedOffset;
}

void Collider::OnCollisionEnter(Collider* other)
{
    if (owner_)
    {
        owner_->OnCollisionEnter(this, other);
    }
}

void Collider::OnCollisionStay(Collider* other)
{
    if (owner_)
    {
        owner_->OnCollisionStay(this, other);
    }
}

void Collider::OnCollisionExit(Collider* other)
{
    if (owner_)
    {
        owner_->OnCollisionExit(this, other);
    }
}

// デバッグ用コライダー描画
void Collider::DrawCollider()
{
    Vector3 center = GetWorldPosition();

    // 球コライダー描画
    if (type_ == CollisionShapeType::Sphere)
    {
        DebugDraw::DrawSphere(center, radius_, color_);
    }
    // AABBコライダー描画
    else if (type_ == CollisionShapeType::AABB)
    {
        DebugDraw::DrawAABB(center - size_, center + size_, color_);
    }
}

}