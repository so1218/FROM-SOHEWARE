#include "CollisionConfig.h" 
#include "CollisionManager.h"
#include "Collision.h"
#include "Vector3.h" 
#include <algorithm>

void CollisionManager::AddCollider(Collider* collider)
{
    // 有効なコライダーのみ登録
    if (collider)
    {
        colliders_.push_back(collider);
    }
}

void CollisionManager::CheckCollisionPair(Collider* colliderA, Collider* colliderB) 
{
    // 無効チェック
    if (!colliderA || !colliderB)
    {
        return;
    }

    // 衝突フィルタリング
    if (((colliderA->GetCollisionAttribute() & colliderB->GetCollisionMask()) == 0) ||
        ((colliderB->GetCollisionAttribute() & colliderA->GetCollisionMask()) == 0))
    {
        return; 
    }

    CollisionShapeType typeA = colliderA->GetType();
    CollisionShapeType typeB = colliderB->GetType();

    // 両方とも球
    if (typeA == CollisionShapeType::Sphere && typeB == CollisionShapeType::Sphere)
    {
        if (IsCollision(colliderA->GetWorldPosition(), colliderA->GetRadius(),
            colliderB->GetWorldPosition(), colliderB->GetRadius())) {
            colliderA->OnCollision(colliderB);
            colliderB->OnCollision(colliderA);
        }
    }
    // 両方ともAABB
    else if (typeA == CollisionShapeType::AABB && typeB == CollisionShapeType::AABB) 
    {
        Vector3 posA = colliderA->GetWorldPosition();
        Vector3 posB = colliderB->GetWorldPosition();
        Vector3 sizeA = colliderA->GetSize();
        Vector3 sizeB = colliderB->GetSize();

        AABB boxA = { posA - sizeA, posA + sizeA }; 
        AABB boxB = { posB - sizeB, posB + sizeB };

        if (IsCollision(boxA, boxB)) {
            colliderA->OnCollision(colliderB);
            colliderB->OnCollision(colliderA);
        }
    }
    // 球とAABB
    else if (typeA == CollisionShapeType::Sphere && typeB == CollisionShapeType::AABB)
    {
        Vector3 posB = colliderB->GetWorldPosition();
        Vector3 sizeB = colliderB->GetSize();
        AABB boxB = { posB - sizeB, posB + sizeB };

        if (IsCollision(boxB, colliderA->GetWorldPosition(), colliderA->GetRadius()))
        {
            colliderA->OnCollision(colliderB);
            colliderB->OnCollision(colliderA);
        }
    }
    // AABBと球
    else if (typeA == CollisionShapeType::AABB && typeB == CollisionShapeType::Sphere)
    {
        Vector3 posA = colliderA->GetWorldPosition();
        Vector3 sizeA = colliderA->GetSize();
        AABB boxA = { posA - sizeA, posA + sizeA };

        if (IsCollision(boxA, colliderB->GetWorldPosition(), colliderB->GetRadius()))
        {
            colliderA->OnCollision(colliderB);
            colliderB->OnCollision(colliderA);
        }
    }
}

void CollisionManager::CheckAllCollisions()
{
    // 登録コライダーを総当たりで判定
    for (auto itrA = colliders_.begin(); itrA != colliders_.end(); ++itrA)
    {
        auto itrB = itrA;
        ++itrB;

        for (; itrB != colliders_.end(); ++itrB)
        {
            CheckCollisionPair(*itrA, *itrB);
        }
    }
}