#pragma once
#include "GameObject.h"

namespace FS
{

class CollisionManager;

class GameObjectManager
{
public:
    void Initialize();
    void Update();
    void Draw();
    void DebugDraw();

    void AddObject(std::unique_ptr<GameObject> obj);

    // 生成関数
    template <typename T, typename... Args>
    T* Create(Args&&... args)
    {
        auto obj = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = obj.get();
        AddObject(std::move(obj));
        return ptr;
    }

    // タグで探す
    GameObject* FindObjectWithTag(uint32_t tag);
    std::vector<GameObject*> FindObjectsWithTag(uint32_t tag);

    // タグで探して、型付きで返す
    template <typename T>
    T* FindObjectWithTag(uint32_t tag)
    {
        return dynamic_cast<T*>(FindObjectWithTag(tag));
    }

    // 型で探す 
    template <typename T>
    T* FindObject()
    {
        for (auto& obj : objects_)
        {
            if (!IsAlive(obj.get())) continue;
            if (auto* p = dynamic_cast<T*>(obj.get())) return p;
        }
        return nullptr;
    }

    template <typename T>
    std::vector<T*> FindObjects()
    {
        std::vector<T*> result;
        for (auto& obj : objects_)
        {
            if (!IsAlive(obj.get())) continue;
            if (auto* p = dynamic_cast<T*>(obj.get())) result.push_back(p);
        }
        return result;
    }

    // ID から取得
    template <typename T = GameObject>
    T* Get(uint64_t id) const
    {
        auto it = idMap_.find(id);
        if (it == idMap_.end() || it->second->IsDead()) return nullptr;
        return dynamic_cast<T*>(it->second);
    }

    void SetCollisionManager(CollisionManager* cm) { collisionManager_ = cm; }
    CollisionManager* GetCollisionManager() const { return collisionManager_; }

    void RequestSort() { isSortNeeded_ = true; }

private:
    static bool IsAlive(const GameObject* obj) { return !obj->IsDead() && obj->IsActive(); }

    void SetupNewObject(GameObject* obj);   // 初期化・行列更新・コライダー登録
    void RemoveDeadObjects();
    void MergePendingObjects();

    std::vector<std::unique_ptr<GameObject>> objects_;
    std::vector<std::unique_ptr<GameObject>> pendingObjects_;   // 追加待機リスト
    std::unordered_map<uint64_t, GameObject*> idMap_;           // IDからオブジェクト
    uint64_t nextId_ = 1;                              

    bool isUpdating_ = false;     // 現在 Update ループ中か
    bool isSortNeeded_ = false;   // 次の Update で並べ替えるか
    bool isInitialized_ = false;  // シーンの Initialize が済んだか
    CollisionManager* collisionManager_ = nullptr;
};

}