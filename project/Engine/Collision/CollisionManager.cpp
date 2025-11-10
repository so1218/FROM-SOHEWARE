#include "CollisionConfig.h" 
#include "CollisionManager.h"
#include "Collision.h"
#include "Vector3.h" 
#include <algorithm>

void CollisionManager::AddCollider(Collider* collider)
{
    if (collider) // nullチェック
    {
        colliders_.push_back(collider);
    }
}

void CollisionManager::CheckCollisionPair(Collider* colliderA, Collider* colliderB) 
{
    if (!colliderA || !colliderB)
    {
        return;
    }

    // 衝突フィルタリング
    if (((colliderA->GetCollisionAttribute() & colliderB->GetCollisionMask()) == 0) ||
        ((colliderB->GetCollisionAttribute() & colliderA->GetCollisionMask()) == 0))
    {
        return; // フィルタリングにより衝突判定をスキップ
    }

    // 球と球の交差判定
    if (IsCollision(colliderA->GetWorldPosition(), colliderA->GetRadius(),
        colliderB->GetWorldPosition(), colliderB->GetRadius()))
    {
        // 衝突したらコールバックを呼び出す
        colliderA->OnCollision(colliderB);
        colliderB->OnCollision(colliderA);
    }
}

void CollisionManager::CheckAllCollisions() 
{
    // リスト内のペアを総当たり
    std::list<Collider*>::iterator itrA = colliders_.begin();
    for (; itrA != colliders_.end(); ++itrA)
    {
        // itrBはitrAの次の要素から開始
        std::list<Collider*>::iterator itrB = itrA;
        ++itrB;
        for (; itrB != colliders_.end(); ++itrB)
        {
            // ペアの衝突判定を呼び出す
            CheckCollisionPair(*itrA, *itrB);
        }
    }
}