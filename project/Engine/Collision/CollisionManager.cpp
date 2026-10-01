#include "pch.h"
#include "CollisionConfig.h" 
#include "CollisionManager.h"
#include "Collision.h"
#include "Collider.h"
#include "GameObject.h"

namespace FE
{

void CollisionManager::ClearColliders()
{
    // リストを空にする前に、全コライダーのマネージャー参照を切る
    for (Collider* collider : colliders_)
    {
        collider->SetManager(nullptr);
    }

    colliders_.clear();
}

void CollisionManager::Reset()
{
    ClearColliders();
    previousCollisionPairs_.clear();
}

void CollisionManager::AddCollider(Collider* collider)
{
    // 有効なコライダーのみ登録
    if (collider)
    {
        colliders_.push_back(collider);

        // コライダーに自分のマネージャーを教える
        collider->SetManager(this);
    }
}

bool CollisionManager::CheckCollisionPair(Collider* colliderA, Collider* colliderB)
{
    CollisionShapeType typeA = colliderA->GetType();
    CollisionShapeType typeB = colliderB->GetType();

    // 球と球
    if (typeA == CollisionShapeType::Sphere && typeB == CollisionShapeType::Sphere)
    {
        return IsCollision(
            colliderA->GetWorldPosition(), colliderA->GetRadius(),
            colliderB->GetWorldPosition(), colliderB->GetRadius()
        );
    }
    // AABBとAABB
    else if (typeA == CollisionShapeType::AABB && typeB == CollisionShapeType::AABB)
    {
        Vector3 posA = colliderA->GetWorldPosition();
        Vector3 posB = colliderB->GetWorldPosition();
        Vector3 sizeA = colliderA->GetSize();
        Vector3 sizeB = colliderB->GetSize();
        AABB boxA = { posA - sizeA, posA + sizeA };
        AABB boxB = { posB - sizeB, posB + sizeB };
        return IsCollision(boxA, boxB);
    }
    // 球とAABB
    else if (typeA == CollisionShapeType::Sphere && typeB == CollisionShapeType::AABB)
    {
        Vector3 posB = colliderB->GetWorldPosition();
        Vector3 sizeB = colliderB->GetSize();
        AABB boxB = { posB - sizeB, posB + sizeB };
        return IsCollision(boxB, colliderA->GetWorldPosition(), colliderA->GetRadius());
    }
    // AABBと球
    else if (typeA == CollisionShapeType::AABB && typeB == CollisionShapeType::Sphere)
    {
        Vector3 posA = colliderA->GetWorldPosition();
        Vector3 sizeA = colliderA->GetSize();
        AABB boxA = { posA - sizeA, posA + sizeA };
        return IsCollision(boxA, colliderB->GetWorldPosition(), colliderB->GetRadius());
    }

    return false;
}

void CollisionManager::CheckAllCollisions()
{
    std::set<CollisionPair> currentCollisionPairs;

    // イテレータより安全で速いインデックスベースのループに変更
    for (size_t i = 0; i < colliders_.size(); ++i)
    {
        Collider* colliderA = colliders_[i];

        if (!colliderA->IsEnable() || (colliderA->GetOwner() && !colliderA->GetOwner()->IsActive()))
        {
            continue;
        }

        for (size_t j = i + 1; j < colliders_.size(); ++j)
        {
            Collider* colliderB = colliders_[j];

            if (!colliderB->IsEnable() || (colliderB->GetOwner() && !colliderB->GetOwner()->IsActive()))
            {
                continue;
            }

            if (((colliderA->GetCollisionAttribute() & colliderB->GetCollisionMask()) == 0) ||
                ((colliderB->GetCollisionAttribute() & colliderA->GetCollisionMask()) == 0))
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
            pair.first->OnCollisionExit(pair.second);
            pair.second->OnCollisionExit(pair.first);
        }
    }

    for (const auto& pair : currentCollisionPairs)
    {
        if (previousCollisionPairs_.find(pair) != previousCollisionPairs_.end())
        {
            pair.first->OnCollisionStay(pair.second);
            pair.second->OnCollisionStay(pair.first);
        }
        else
        {
            pair.first->OnCollisionEnter(pair.second);
            pair.second->OnCollisionEnter(pair.first);
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

    collider->SetManager(nullptr);
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

bool CollisionManager::RaycastAABB(const Vector3& rayOrigin, const Vector3& rayDir, const Vector3& boxMin, const Vector3& boxMax, float& outT, Vector3& outNormal)
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
    uint32_t targetMask)
{
    bool hasHit = false;
    float closestT = maxDistance;
    Vector3 normalizedDir = rayDirection.Normalize();

    for (Collider* collider : colliders_)
    {
        // 無効なコライダーや非アクティブなオブジェクトはスキップ
        if (!collider || !collider->IsEnable()) continue;
        if (collider->GetOwner() && !collider->GetOwner()->IsActive()) continue;

        // 衝突属性とマスクのチェック
        if ((collider->GetCollisionAttribute() & targetMask) == 0) continue;

        float t = 0.0f;
        Vector3 normal = { 0.0f, 0.0f, 0.0f };
        bool isHit = false;

        Vector3 pos = collider->GetWorldPosition();

        if (collider->GetType() == CollisionShapeType::Sphere)
        {
            isHit = RaycastSphere(rayOrigin, normalizedDir, pos, collider->GetRadius(), t, normal);
        }
        else if (collider->GetType() == CollisionShapeType::AABB)
        {
            Vector3 size = collider->GetSize();
            Vector3 boxMin = pos - size;
            Vector3 boxMax = pos + size;
            isHit = RaycastAABB(rayOrigin, normalizedDir, boxMin, boxMax, t, normal);
        }

        // 最も手前（距離 t が一番小さい）衝突相手を記憶
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