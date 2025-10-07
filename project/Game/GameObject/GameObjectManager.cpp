#include "GameObjectManager.h"

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
    std::sort(objects_.begin(), objects_.end(),
        [](const std::unique_ptr<GameObject>& a, const std::unique_ptr<GameObject>& b) 
        {
            return a->GetDrawPriority() < b->GetDrawPriority();
        });

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

void GameObjectManager::ApplyGlobalVariables()
{
    for (auto& obj : objects_)
    {
        obj->ApplyGlobalVariables();
    }
}