#include "pch.h"
#include "CollisionConfig.h" 
#include "CollisionManager.h"
#include "Collision.h"
#include "Collider.h"

namespace FE
{

void CollisionManager::AddCollider(Collider* collider)
{
    // 有効なコライダーのみ登録
    if (collider)
    {
        colliders_.push_back(collider);
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
    // 今回のフレームで衝突しているペアのリスト
    std::set<CollisionPair> currentCollisionPairs;

    for (auto itrA = colliders_.begin(); itrA != colliders_.end(); ++itrA)
    {
        auto itrB = itrA;
        ++itrB;
        for (; itrB != colliders_.end(); ++itrB)
        {
            Collider* colliderA = *itrA;
            Collider* colliderB = *itrB;

            // フィルタリング
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

    // Exit判定
    for (const auto& pair : previousCollisionPairs_)
    {
        // 今回のリストに存在しない
        if (currentCollisionPairs.find(pair) == currentCollisionPairs.end())
        {
            // ポインタが有効かチェックする
            // colliders_リストの中にポインタがあれば、まだdeleteされていない
            bool isAliveA = false;
            bool isAliveB = false;

            // リストを検索して生存確認
            for (Collider* collider : colliders_)
            {
                if (collider == pair.first) isAliveA = true;
                if (collider == pair.second) isAliveB = true;
            }

            // Aが生きていればExitを呼ぶ
            if (isAliveA)
            {
                pair.first->OnCollisionExit(pair.second);
            }
            // Bが生きていればExitを呼ぶ
            if (isAliveB)
            {
                pair.second->OnCollisionExit(pair.first);
            }
        }
    }

    // EnterとStay判定
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

    // 履歴の更新
    previousCollisionPairs_ = currentCollisionPairs;
}

}