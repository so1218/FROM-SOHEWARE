#pragma once
#include <memory>  
#include <vector>

#include "GameObject.h"

class CollisionManager;

class GameObjectManager 
{
public:
    void Initialize();
    void Update();
    void Draw();
    void DebugDraw();

    void AddObject(std::unique_ptr<GameObject> obj);

    void AddAllCollidersToManager(CollisionManager* manager);

    // 生成関数
    template <typename T, typename... Args>
    T* Create(Args&&... args)
    {
        // オブジェクト生成
        auto obj = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = obj.get();

        // リストに追加
        objects_.push_back(std::move(obj));

        // ソートが必要フラグを立てる
        isSortNeeded_ = true;

        return ptr;
    }

    // 特定のタグを持つオブジェクトを1つ探す
    GameObject* FindObjectWithTag(const std::string& tag);

    // 特定のタグを持つオブジェクトを全てリストアップ
    std::vector<GameObject*> FindGameObjectsWithTag(const std::string& tag);

private:
    std::vector<std::unique_ptr<GameObject>> objects_;
    bool isSortNeeded_ = false; // 毎フレームソートしないためのフラグ
};