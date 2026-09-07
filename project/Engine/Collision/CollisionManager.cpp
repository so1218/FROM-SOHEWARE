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

}