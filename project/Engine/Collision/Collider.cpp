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

bool Collider::CalculatePushBackVector(Collider* other, FE::Vector3& outPushVector) const
{
    outPushVector = { 0.0f, 0.0f, 0.0f };
    if (!other) return false;

    FE::Vector3 myPos = GetWorldPosition();
    FE::Vector3 otherPos = other->GetWorldPosition();

    // ========================================================
    // AABB(自分) vs AABB(相手)
    // ========================================================
    if (type_ == CollisionShapeType::AABB && other->GetType() == CollisionShapeType::AABB)
    {
        FE::Vector3 diff = otherPos - myPos; // 自分から相手へのベクトル

        // 各軸の重なり具合（めり込み量）を計算
        float overlapX = (size_.x + other->GetSize().x) - std::abs(diff.x);
        float overlapY = (size_.y + other->GetSize().y) - std::abs(diff.y);
        float overlapZ = (size_.z + other->GetSize().z) - std::abs(diff.z);

        if (overlapX > 0.0f && overlapY > 0.0f && overlapZ > 0.0f)
        {
            // 最もめり込みが浅い軸へ押し出す
            if (overlapX <= overlapY && overlapX <= overlapZ) {
                outPushVector.x = (diff.x > 0.0f) ? overlapX : -overlapX;
            }
            else if (overlapY <= overlapX && overlapY <= overlapZ) {
                outPushVector.y = (diff.y > 0.0f) ? overlapY : -overlapY;
            }
            else {
                outPushVector.z = (diff.z > 0.0f) ? overlapZ : -overlapZ;
            }
            return true;
        }
    }
    // ========================================================
    // Sphere(自分) vs AABB(相手)
    // ========================================================
    else if (type_ == CollisionShapeType::Sphere && other->GetType() == CollisionShapeType::AABB)
    {
        FE::Vector3 boxMin = otherPos - other->GetSize();
        FE::Vector3 boxMax = otherPos + other->GetSize();

        // AABB(相手)の表面または内部における、Sphere中心(自分)からの最近接点
        FE::Vector3 closest;
        closest.x = std::clamp(myPos.x, boxMin.x, boxMax.x);
        closest.y = std::clamp(myPos.y, boxMin.y, boxMax.y);
        closest.z = std::clamp(myPos.z, boxMin.z, boxMax.z);

        FE::Vector3 diff = closest - myPos; // 自分(Sphere)から最近接点へのベクトル
        float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;

        if (distSq < radius_ * radius_)
        {
            if (distSq > 0.0001f) {
                float dist = std::sqrt(distSq);
                float overlap = radius_ - dist;
                outPushVector = (diff / dist) * overlap; // 相手を外側へ押し出す
            }
            return true;
        }
    }
    // ========================================================
    // AABB(自分) vs Sphere(相手)
    // ========================================================
    else if (type_ == CollisionShapeType::AABB && other->GetType() == CollisionShapeType::Sphere)
    {
        FE::Vector3 boxMin = myPos - size_;
        FE::Vector3 boxMax = myPos + size_;

        // AABB(自分)の表面または内部における、Sphere中心(相手)からの最近接点
        FE::Vector3 closest;
        closest.x = std::clamp(otherPos.x, boxMin.x, boxMax.x);
        closest.y = std::clamp(otherPos.y, boxMin.y, boxMax.y);
        closest.z = std::clamp(otherPos.z, boxMin.z, boxMax.z);

        FE::Vector3 diff = otherPos - closest; // 最近接点から相手(Sphere)へのベクトル
        float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;

        if (distSq < other->GetRadius() * other->GetRadius())
        {
            if (distSq > 0.0001f) {
                float dist = std::sqrt(distSq);
                float overlap = other->GetRadius() - dist;
                outPushVector = (diff / dist) * overlap;
            }
            return true;
        }
    }
    // ========================================================
    // Sphere(自分) vs Sphere(相手)
    // ========================================================
    else if (type_ == CollisionShapeType::Sphere && other->GetType() == CollisionShapeType::Sphere)
    {
        FE::Vector3 diff = otherPos - myPos;
        float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
        float minDist = radius_ + other->GetRadius();

        if (distSq < minDist * minDist)
        {
            float dist = std::sqrt(distSq);
            if (dist > 0.0001f) {
                float overlap = minDist - dist;
                outPushVector = (diff / dist) * overlap;
            }
            return true;
        }
    }

    return false;
}

}