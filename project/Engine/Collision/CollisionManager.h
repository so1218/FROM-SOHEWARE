#pragma once

namespace FE
{

class Collider;

// コライダー同士の衝突判定を管理するクラス
class CollisionManager
{
public:
    // 登録済みコライダーを全てクリア
    void ClearColliders();

    void Reset();

    // コライダーを登録
    void AddCollider(Collider* collider);

    // 全コライダーの衝突判定を実行
    void CheckAllCollisions();

    // コライダーの登録を解除
    void RemoveCollider(Collider* collider);

private:
    // 登録コライダーのリスト
    std::vector<Collider*> colliders_;

    // コライダーペアの衝突判定
    bool CheckCollisionPair(Collider* colliderA, Collider* colliderB);

    // コライダーのペアを型定義（ポインタの大小で保存順を固定）
    using CollisionPair = std::pair<Collider*, Collider*>;

    // 前フレームで衝突していたペアのリスト
    std::set<CollisionPair> previousCollisionPairs_;
};

}
