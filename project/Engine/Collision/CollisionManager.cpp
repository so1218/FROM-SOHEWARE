#include "pch.h"
#include "CollisionManager.h"
#include "Collider.h"
#include "GameObject.h"
#include "DebugDraw.h"

namespace FS
{

namespace
{

bool IsSphereOverlap(const Vector3& centerA, float radiusA, const Vector3& centerB, float radiusB)
{
    const float radiusSum = radiusA + radiusB;
    return (centerB - centerA).LengthSq() < radiusSum * radiusSum;
}

// halfSize は中心から面までの距離
bool IsBoxOverlap(const Vector3& centerA, const Vector3& halfSizeA, const Vector3& centerB, const Vector3& halfSizeB)
{
    const Vector3 diff = centerB - centerA;
    return std::abs(diff.x) < halfSizeA.x + halfSizeB.x
        && std::abs(diff.y) < halfSizeA.y + halfSizeB.y
        && std::abs(diff.z) < halfSizeA.z + halfSizeB.z;
}

bool IsBoxSphereOverlap(const Vector3& boxCenter, const Vector3& halfSize, const Vector3& sphereCenter, float radius)
{
    // 球の中心に一番近い箱の上の点
    const Vector3 boxMin = boxCenter - halfSize;
    const Vector3 boxMax = boxCenter + halfSize;
    const Vector3 closest =
    {
        std::clamp(sphereCenter.x, boxMin.x, boxMax.x),
        std::clamp(sphereCenter.y, boxMin.y, boxMax.y),
        std::clamp(sphereCenter.z, boxMin.z, boxMax.z),
    };
    return (sphereCenter - closest).LengthSq() < radius * radius;
}

}

void CollisionManager::ResetCollisionHistory()
{
    previousCollisionPairs_.clear();
}

void CollisionManager::AddCollider(Collider* collider)
{
    if (collider)
    {
        colliders_.push_back(collider);
    }
}

bool CollisionManager::CheckCollisionPair(Collider* colliderA, Collider* colliderB)
{
    const Vector3 centerA = colliderA->GetWorldPosition();
    const Vector3 centerB = colliderB->GetWorldPosition();
    const bool isSphereA = colliderA->GetType() == CollisionShapeType::Sphere;
    const bool isSphereB = colliderB->GetType() == CollisionShapeType::Sphere;

    if (isSphereA && isSphereB)
    {
        return IsSphereOverlap(centerA, colliderA->GetRadius(), centerB, colliderB->GetRadius());
    }
    if (!isSphereA && !isSphereB)
    {
        return IsBoxOverlap(centerA, colliderA->GetHalfSize(), centerB, colliderB->GetHalfSize());
    }
    if (isSphereA)
    {
        return IsBoxSphereOverlap(centerB, colliderB->GetHalfSize(), centerA, colliderA->GetRadius());
    }
    return IsBoxSphereOverlap(centerA, colliderA->GetHalfSize(), centerB, colliderB->GetRadius());
}

void CollisionManager::CheckAllCollisions()
{
    std::set<CollisionPair> currentCollisionPairs;

    // イテレータより安全で速いインデックスベースのループに変更
    for (size_t i = 0; i < colliders_.size(); ++i)
    {
        Collider* colliderA = colliders_[i];

        if (!colliderA->IsEnable() || colliderA->IsRaycastOnly() || (colliderA->GetOwner() && !colliderA->GetOwner()->IsActive()))
        {
            continue;
        }

        for (size_t j = i + 1; j < colliders_.size(); ++j)
        {
            Collider* colliderB = colliders_[j];

            if (!colliderB->IsEnable() || colliderB->IsRaycastOnly() || (colliderB->GetOwner() && !colliderB->GetOwner()->IsActive()))
            {
                continue;
            }

            if (((colliderA->GetCollisionLayer() & colliderB->GetCollisionMask()) == 0) ||
                ((colliderB->GetCollisionLayer() & colliderA->GetCollisionMask()) == 0))
            {
                continue;
            }

            if (CheckCollisionPair(colliderA, colliderB))
            {
                CollisionPair pair;
                if (colliderA < colliderB) pair = { colliderA, colliderB };
                else                       pair = { colliderB, colliderA };
                currentCollisionPairs.insert(pair);
            }
        }
    }

    for (const auto& pair : previousCollisionPairs_)
    {
        if (currentCollisionPairs.find(pair) == currentCollisionPairs.end())
        {
            NotifyExit(pair.first, pair.second);
            NotifyExit(pair.second, pair.first);
        }
    }

    for (const auto& pair : currentCollisionPairs)
    {
        if (previousCollisionPairs_.find(pair) != previousCollisionPairs_.end())
        {
            NotifyStay(pair.first, pair.second);
            NotifyStay(pair.second, pair.first);
        }
        else
        {
            NotifyEnter(pair.first, pair.second);
            NotifyEnter(pair.second, pair.first);
        }
    }

    previousCollisionPairs_ = std::move(currentCollisionPairs); 
}

void CollisionManager::RemoveCollider(Collider* collider)
{
    if (!collider) return;

    // 一番後ろの要素と入れ替えてから末尾を削除
    auto it = std::find(colliders_.begin(), colliders_.end(), collider);
    if (it != colliders_.end())
    {
        std::swap(*it, colliders_.back());
        colliders_.pop_back();
    }

    for (auto itHistory = previousCollisionPairs_.begin(); itHistory != previousCollisionPairs_.end(); )
    {
        if (itHistory->first == collider || itHistory->second == collider)
        {
            itHistory = previousCollisionPairs_.erase(itHistory);
        }
        else
        {
            ++itHistory;
        }
    }
}

void CollisionManager::NotifyEnter(Collider* mine, Collider* other)
{
    if (GameObject* owner = mine->GetOwner()) owner->OnCollisionEnter(mine, other);
}

void CollisionManager::NotifyStay(Collider* mine, Collider* other)
{
    if (GameObject* owner = mine->GetOwner()) owner->OnCollisionStay(mine, other);
}

void CollisionManager::NotifyExit(Collider* mine, Collider* other)
{
    if (GameObject* owner = mine->GetOwner()) owner->OnCollisionExit(mine, other);
}

void CollisionManager::DrawColliders() const
{
    for (const Collider* collider : colliders_)
    {
        const GameObject* owner = collider->GetOwner();
        if (owner && !owner->IsActive()) continue;

        const Vector3 center = collider->GetWorldPosition();
        if (collider->GetType() == CollisionShapeType::Sphere)
        {
            DebugDraw::DrawSphere(center, collider->GetRadius(), collider->GetColor());
        }
        else
        {
            const Vector3 halfSize = collider->GetHalfSize();
            DebugDraw::DrawAABB(center - halfSize, center + halfSize, collider->GetColor());
        }
    }
}

bool CollisionManager::RaycastSphere(const Vector3& rayOrigin, const Vector3& rayDir, const Vector3& sphereCenter, float sphereRadius, float& outT, Vector3& outNormal)
{
    Vector3 oc = rayOrigin - sphereCenter;
    float b = oc.Dot(rayDir);
    float c = oc.Dot(oc) - sphereRadius * sphereRadius;
    float discriminant = b * b - c;

    if (discriminant < 0.0f) return false;

    float sqrtD = std::sqrt(discriminant);
    float t = -b - sqrtD;

    if (t < 0.0f)
    {
        t = -b + sqrtD;
        if (t < 0.0f) return false;
    }

    outT = t;
    Vector3 hitPoint = rayOrigin + rayDir * t;
    outNormal = (hitPoint - sphereCenter).Normalize();
    return true;
}

bool CollisionManager::RaycastBox(const Vector3& rayOrigin, const Vector3& rayDir, const Vector3& boxMin, const Vector3& boxMax, float& outT, Vector3& outNormal)
{
    float tMin = 0.0f;
    float tMax = FLT_MAX;
    Vector3 hitNormal = { 0.0f, 0.0f, 0.0f };

    float origin[3] = { rayOrigin.x, rayOrigin.y, rayOrigin.z };
    float dir[3] = { rayDir.x, rayDir.y, rayDir.z };
    float minB[3] = { boxMin.x, boxMin.y, boxMin.z };
    float maxB[3] = { boxMax.x, boxMax.y, boxMax.z };

    for (int i = 0; i < 3; ++i)
    {
        if (std::abs(dir[i]) < 0.00001f)
        {
            if (origin[i] < minB[i] || origin[i] > maxB[i]) return false;
        }
        else
        {
            float invD = 1.0f / dir[i];
            float t0 = (minB[i] - origin[i]) * invD;
            float t1 = (maxB[i] - origin[i]) * invD;

            Vector3 n0 = { i == 0 ? -1.0f : 0.0f, i == 1 ? -1.0f : 0.0f, i == 2 ? -1.0f : 0.0f };
            Vector3 n1 = { i == 0 ? 1.0f : 0.0f, i == 1 ? 1.0f : 0.0f, i == 2 ? 1.0f : 0.0f };

            if (invD < 0.0f)
            {
                std::swap(t0, t1);
                std::swap(n0, n1);
            }

            if (t0 > tMin)
            {
                tMin = t0;
                hitNormal = n0;
            }
            if (t1 < tMax) tMax = t1;

            if (tMax < tMin) return false;
        }
    }

    outT = tMin;
    outNormal = hitNormal;
    return true;
}

bool CollisionManager::Raycast(
    const Vector3& rayOrigin,
    const Vector3& rayDirection,
    float maxDistance,
    RaycastHit* outHit,
    uint32_t layerMask)
{
    bool hasHit = false;
    float closestT = maxDistance;
    Vector3 normalizedDir = rayDirection.Normalize();

    for (Collider* collider : colliders_)
    {
        // 無効なコライダーや非アクティブなオブジェクトはスキップ
        if (!collider || !collider->IsEnable()) continue;
        if (collider->GetOwner() && !collider->GetOwner()->IsActive()) continue;

        if ((collider->GetCollisionLayer() & layerMask) == 0) continue;

        float t = 0.0f;
        Vector3 normal = { 0.0f, 0.0f, 0.0f };
        bool isHit = false;

        Vector3 pos = collider->GetWorldPosition();

        if (collider->GetType() == CollisionShapeType::Sphere)
        {
            isHit = RaycastSphere(rayOrigin, normalizedDir, pos, collider->GetRadius(), t, normal);
        }
        else if (collider->GetType() == CollisionShapeType::Box)
        {
            Vector3 size = collider->GetHalfSize();
            Vector3 boxMin = pos - size;
            Vector3 boxMax = pos + size;
            isHit = RaycastBox(rayOrigin, normalizedDir, boxMin, boxMax, t, normal);
        }

        // 最も手前衝突相手を記憶
        if (isHit && t >= 0.0f && t < closestT)
        {
            closestT = t;
            hasHit = true;

            if (outHit)
            {
                outHit->hitCollider = collider;
                outHit->hitObject = collider->GetOwner();
                outHit->point = rayOrigin + normalizedDir * t;
                outHit->normal = normal;
                outHit->distance = t;
            }
        }
    }

    return hasHit;
}

}