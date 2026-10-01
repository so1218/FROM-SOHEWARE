#pragma once
#include "GameObject.h"

namespace FE
{

class Collider;

struct RaycastHit
{
    Collider* hitCollider = nullptr;   // ヒットしたコライダー
    GameObject* hitObject = nullptr;   // ヒットしたオブジェクト（親）
    Vector3 point = { 0.0f, 0.0f, 0.0f };  // 着弾ワールド座標
    Vector3 normal = { 0.0f, 0.0f, 1.0f }; // 着弾面の法線ベクトル
    float distance = 0.0f;             // 射出位置からの距離
};

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

    // レイキャスト判定（登録中の全コライダーを走査して最も近い衝突対象を取得）
    bool Raycast(
        const Vector3& rayOrigin,
        const Vector3& rayDirection,
        float maxDistance,
        RaycastHit* outHit,
        uint32_t targetMask = 0xFFFFFFFF
    );

private:
    // 登録コライダーのリスト
    std::vector<Collider*> colliders_;

    // コライダーペアの衝突判定
    bool CheckCollisionPair(Collider* colliderA, Collider* colliderB);

    // コライダーのペアを型定義（ポインタの大小で保存順を固定）
    using CollisionPair = std::pair<Collider*, Collider*>;

    // 前フレームで衝突していたペアのリスト
    std::set<CollisionPair> previousCollisionPairs_;

    // Ray vs Sphere（球）判定
    bool RaycastSphere(const Vector3& rayOrigin, const Vector3& rayDir, const Vector3& sphereCenter, float sphereRadius, float& outT, Vector3& outNormal);
    // Ray vs AABB（軸平行ボックス）判定
    bool RaycastAABB(const Vector3& rayOrigin, const Vector3& rayDir, const Vector3& boxMin, const Vector3& boxMax, float& outT, Vector3& outNormal);
};

}
