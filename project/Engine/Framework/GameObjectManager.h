#pragma once
#include "GameObject.h"

namespace FE
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
        // オブジェクト生成
        auto obj = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = obj.get();

        // あとの処理は全てAddObjectに
        AddObject(std::move(obj));

        return ptr;
    }

    // 特定のタグを持つオブジェクトを1つ探す
    GameObject* FindObjectWithTag(uint32_t tag);

    // 特定のタグを持つオブジェクトを全てリストアップ
    std::vector<GameObject*> FindObjectsWithTag(uint32_t tag);

    void SetCollisionManager(CollisionManager* cm) { collisionManager_ = cm; }
    CollisionManager* GetCollisionManager() const { return collisionManager_; }

private:
    std::vector<std::unique_ptr<GameObject>> objects_;
    std::vector<std::unique_ptr<GameObject>> pendingObjects_; // 追加待機リスト
    bool isUpdating_ = false; // 現在Updateループ中かどうかのフラグ
    bool isSortNeeded_ = false; // 毎フレームソートしないためのフラグ
    CollisionManager* collisionManager_ = nullptr;
};

}