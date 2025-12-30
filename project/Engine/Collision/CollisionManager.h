#pragma once
#include "Collider.h"

#include <list>

// コライダー同士の衝突判定を管理するクラス
class CollisionManager
{
public:
    // 登録済みコライダーを全てクリア
    void ClearColliders() { colliders_.clear(); }

    // コライダーを登録
    void AddCollider(Collider* collider);

    // 全コライダーの衝突判定を実行
    void CheckAllCollisions();

private:
    // 登録コライダーのリスト
    std::list<Collider*> colliders_;

    // コライダーペアの衝突判定
    void CheckCollisionPair(Collider* colliderA, Collider* colliderB);
};