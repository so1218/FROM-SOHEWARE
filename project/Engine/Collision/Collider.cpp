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
        // 親が存在しない場合はオフセットを返す
        if (!owner_) return centerOffset_;

        const WorldTransform& transform = owner_->GetTransform();

        // フラグを見て、回転を適用するかどうかを変える
        if (applyRotation_)
        {
            // 回転を適用する
            Vector3 rotatedOffset = transform.rotationQuaternion_.RotateVector(centerOffset_);
            return transform.GetWorldPosition() + rotatedOffset;
        }
        else
        {
            // 回転を適用しない
            return transform.GetWorldPosition() + centerOffset_;
        }
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

    void Collider::RegisterToManager()
    {
        if (owner_ && owner_->GetManager())
        {
            manager_ = owner_->GetManager()->GetCollisionManager();
            if (manager_) manager_->AddCollider(this);
        }
    }

}