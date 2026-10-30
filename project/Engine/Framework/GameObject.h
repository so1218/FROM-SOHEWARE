#pragma once
#include "Engine.h"

namespace FS
{ 

class Collider;
class GameObjectManager;

class GameObject 
{
public:
    GameObject();
    virtual ~GameObject();

    virtual void Initialize() {}
    virtual void Update() {}
    virtual void Draw() {}
    virtual void DebugDraw() {}
    virtual void OnDestroy() {}

    // 生存フラグ
    bool IsDead() const { return isDead_; }
    void Destroy() { isDead_ = true; }

    // タグ
    void SetTag(uint32_t tag) { tag_ = tag; }
    uint32_t GetTag() const { return tag_; }
    bool CompareTag(uint32_t tag) const { return tag_ == tag; }

    // 優先度
    void SetUpdatePriority(int priority);
    int GetUpdatePriority() const { return updatePriority_; }

    // Transform
    WorldTransform& GetTransform() { return transform_; }
    const WorldTransform& GetTransform() const { return transform_; }
    void SetTransform(const WorldTransform& transform) { transform_ = transform; }

    // 衝突コールバック
    virtual void OnCollisionEnter(Collider* mine, Collider* other) {}
    virtual void OnCollisionStay(Collider* mine, Collider* other) {}
    virtual void OnCollisionExit(Collider* mine, Collider* other) {}
    
    // コライダーの作成。登録はマネージャーが自動で行う
    Collider* AddCollider();
    // AddCollider() で作ったコライダーを削除する (登録解除も自動で行われる)
    void RemoveCollider(Collider* collider);
    const std::vector<std::unique_ptr<Collider>>& GetColliders() const { return colliders_; }

    // マネージャーへの追加時に 1 回だけ呼ばれる
    void AttachToManager(GameObjectManager* manager, uint64_t id);

    // オブジェクトごとに一意な ID 
    uint64_t GetId() const { return id_; }

    // マネージャー
    GameObjectManager* GetManager() const { return manager_; }

    bool IsActive() const { return isActive_; }
    void SetActive(bool active) { isActive_ = active; }

    // コライダーを CollisionManager に登録
    void RegisterColliders();

private:

    WorldTransform transform_;
    uint32_t tag_ = 0;
    int updatePriority_ = 50;
    bool isDead_ = false;
    bool isActive_ = true;
    uint64_t id_ = 0;
    GameObjectManager* manager_ = nullptr;
    std::vector<std::unique_ptr<Collider>> colliders_;
};

}