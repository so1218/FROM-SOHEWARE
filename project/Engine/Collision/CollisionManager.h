#pragma once
#include "GameObject.h"

namespace FS
{

class Collider;

struct RaycastHit
{
    Collider* hitCollider = nullptr;  
    GameObject* hitObject = nullptr; 
    Vector3 point = { 0.0f, 0.0f, 0.0f }; // 着弾ワールド座標
    Vector3 normal = { 0.0f, 0.0f, 1.0f }; // 着弾面の法線ベクトル
    float distance = 0.0f; // 射出位置からの距離
};

// コライダー同士の衝突判定を管理するクラス
class CollisionManager
{
public:
    // GameObject がコライダーを作ったとき・消したときに呼ぶ。ゲーム側から呼ぶ必要はない
    void AddCollider(Collider* collider);
    void RemoveCollider(Collider* collider);

    // 前フレームまでの衝突の記録を消す。シーンを抜けるときに呼ぶ。
    // コライダーの登録は持ち主の GameObject と同じだけ続くので、ここでは消さない
    void ResetCollisionHistory();

    // 全コライダーの衝突判定を実行
    void CheckAllCollisions();

    // 全コライダーをデバッグ表示する
    void DrawColliders() const;

    // レイに最初に当たったコライダーを返す
    bool Raycast(
        const Vector3& rayOrigin,
        const Vector3& rayDirection,
        float maxDistance,
        RaycastHit* outHit,
        uint32_t layerMask = 0xFFFFFFFF
    );

private:
    // 登録コライダーのリスト
    std::vector<Collider*> colliders_;

    // コライダーペアの衝突判定
    bool CheckCollisionPair(Collider* colliderA, Collider* colliderB);

    // 持ち主の GameObject に衝突を知らせる
    void NotifyEnter(Collider* mine, Collider* other);
    void NotifyStay(Collider* mine, Collider* other);
    void NotifyExit(Collider* mine, Collider* other);

    // コライダーのペアを型定義（ポインタの大小で保存順を固定）
    using CollisionPair = std::pair<Collider*, Collider*>;

    // 前フレームで衝突していたペアのリスト
    std::set<CollisionPair> previousCollisionPairs_;

    // Ray vs Sphere（球）判定
    bool RaycastSphere(const Vector3& rayOrigin, const Vector3& rayDir, const Vector3& sphereCenter, float sphereRadius, float& outT, Vector3& outNormal);
    // Ray vs Box 判定
    bool RaycastBox(const Vector3& rayOrigin, const Vector3& rayDir, const Vector3& boxMin, const Vector3& boxMax, float& outT, Vector3& outNormal);
};

}
