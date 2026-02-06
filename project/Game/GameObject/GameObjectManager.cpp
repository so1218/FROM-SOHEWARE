#include "GameObjectManager.h"
#include "CollisionManager.h" 
#include "Collider.h"
#include "Enemy.h" 
#include <algorithm>

void GameObjectManager::AddObject(std::unique_ptr<GameObject> obj) 
{
    objects_.push_back(std::move(obj));
}

void GameObjectManager::Initialize()
{
    for (auto& obj : objects_)
    {
        obj->Initialize();
    }
}

void GameObjectManager::Update()
{
    std::sort(objects_.begin(), objects_.end(),
        [](const std::unique_ptr<GameObject>& a, const std::unique_ptr<GameObject>& b) 
        {
            return a->GetUpdatePriority() < b->GetUpdatePriority();
        });

    for (auto& obj : objects_) 
    {
        obj->Update();
    }

    // 削除判定して消す
    objects_.erase(
        std::remove_if(objects_.begin(), objects_.end(),
            [](const std::unique_ptr<GameObject>& obj) {
                // GameObjectに IsDead() のようなメソッドを用意しておく
                return obj->IsDead();
            }),
        objects_.end());
}

void GameObjectManager::Draw() 
{
    for (auto& obj : objects_)
    {
        obj->Draw();
    }
}

void GameObjectManager::DebugDraw()
{
    for (auto& obj : objects_)
    {
        obj->DebugDraw();
    }
}

void GameObjectManager::AddAllCollidersToManager(CollisionManager* manager)
{
    // 自分が持っている全てのオブジェクトをループ
    for (const auto& object : objects_)
    {
        // GameObject* を Collider* に動的キャスト
        Collider* collider = dynamic_cast<Collider*>(object.get());

        // キャストが成功し、Collider であれば登録
        if (collider)
        {
            manager->AddCollider(collider);
        }
    }
}
