#include "pch.h"
#include "Collider.h"
#include "GameObject.h"
#include "DevError.h"

namespace FS
{

Collider::Collider(GameObject* owner, CollisionShapeType type) : owner_(owner), type_(type)
{

}

void Collider::SetRadius(float radius, std::source_location location)
{
    if (type_ != CollisionShapeType::Sphere)
    {
        DevError::Report("球ではないコライダーに SetRadius した。箱の大きさは SetSize で指定する", location);
        return;
    }
    radius_ = radius;
}

void Collider::SetSize(const Vector3& size, std::source_location location)
{
    if (type_ != CollisionShapeType::Box)
    {
        DevError::Report("箱ではないコライダーに SetSize した。球の大きさは SetRadius で指定する", location);
        return;
    }
    size_ = size;
}

Vector3 Collider::GetWorldPosition() const
{
    // 親が存在しない場合はオフセットを返す
    if (!owner_) return centerOffset_;

    const WorldTransform& transform = owner_->GetTransform();
    if (isRotateCenterOffset_)
    {
        return transform.GetWorldPosition() + transform.rotation_.RotateVector(centerOffset_);
    }
    return transform.GetWorldPosition() + centerOffset_;
}

bool Collider::CalculatePushBackVector(Collider* other, FS::Vector3& outPushVector) const
{
    outPushVector = { 0.0f, 0.0f, 0.0f };
    if (!other) return false;

    FS::Vector3 myPos = GetWorldPosition();
    const FS::Vector3 halfSize = GetHalfSize();
    FS::Vector3 otherPos = other->GetWorldPosition();

    // ========================================================
    // Box(自分) vs Box(相手)
    // ========================================================
    if (type_ == CollisionShapeType::Box && other->GetType() == CollisionShapeType::Box)
    {
        FS::Vector3 diff = otherPos - myPos;

        // 各軸の重なり具合を計算
        float overlapX = (halfSize.x + other->GetHalfSize().x) - std::abs(diff.x);
        float overlapY = (halfSize.y + other->GetHalfSize().y) - std::abs(diff.y);
        float overlapZ = (halfSize.z + other->GetHalfSize().z) - std::abs(diff.z);

        if (overlapX > 0.0f && overlapY > 0.0f && overlapZ > 0.0f)
        {
            // 最もめり込みが浅い軸へ押し出す
            if (overlapX <= overlapY && overlapX <= overlapZ)
            {
                outPushVector.x = (diff.x > 0.0f) ? overlapX : -overlapX;
            }
            else if (overlapY <= overlapX && overlapY <= overlapZ) 
            {
                outPushVector.y = (diff.y > 0.0f) ? overlapY : -overlapY;
            }
            else 
            {
                outPushVector.z = (diff.z > 0.0f) ? overlapZ : -overlapZ;
            }
            return true;
        }
    }

    // ========================================================
    // Sphere(自分) vs Box(相手)
    // ========================================================
    else if (type_ == CollisionShapeType::Sphere && other->GetType() == CollisionShapeType::Box)
    {
        FS::Vector3 boxMin = otherPos - other->GetHalfSize();
        FS::Vector3 boxMax = otherPos + other->GetHalfSize();

        // Box(相手)の表面または内部における、Sphere中心(自分)からの最近接点
        FS::Vector3 closest;
        closest.x = std::clamp(myPos.x, boxMin.x, boxMax.x);
        closest.y = std::clamp(myPos.y, boxMin.y, boxMax.y);
        closest.z = std::clamp(myPos.z, boxMin.z, boxMax.z);

        FS::Vector3 diff = closest - myPos; // 自分(Sphere)から最近接点へのベクトル
        float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;

        if (distSq < radius_ * radius_)
        {
            if (distSq > 0.0001f)
            {
                float dist = std::sqrt(distSq);
                float overlap = radius_ - dist;
                outPushVector = (diff / dist) * overlap; 
            }
            return true;
        }
    }

    // ========================================================
    // Box(自分) vs Sphere(相手)
    // ========================================================
    else if (type_ == CollisionShapeType::Box && other->GetType() == CollisionShapeType::Sphere)
    {
        FS::Vector3 boxMin = myPos - halfSize;
        FS::Vector3 boxMax = myPos + halfSize;

        // Box(自分)の表面または内部における、Sphere中心(相手)からの最近接点
        FS::Vector3 closest;
        closest.x = std::clamp(otherPos.x, boxMin.x, boxMax.x);
        closest.y = std::clamp(otherPos.y, boxMin.y, boxMax.y);
        closest.z = std::clamp(otherPos.z, boxMin.z, boxMax.z);

        FS::Vector3 diff = otherPos - closest; // 最近接点から相手(Sphere)へのベクトル
        float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;

        if (distSq < other->GetRadius() * other->GetRadius())
        {
            if (distSq > 0.0001f) 
            {
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
        FS::Vector3 diff = otherPos - myPos;
        float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
        float minDist = radius_ + other->GetRadius();

        if (distSq < minDist * minDist)
        {
            float dist = std::sqrt(distSq);
            if (dist > 0.0001f)
            {
                float overlap = minDist - dist;
                outPushVector = (diff / dist) * overlap;
            }
            return true;
        }
    }

    return false;
}

}