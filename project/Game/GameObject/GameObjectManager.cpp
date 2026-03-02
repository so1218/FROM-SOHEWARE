#include "GameObjectManager.h"
#include "CollisionManager.h" 
#include "Collider.h"
#include <algorithm>

void GameObjectManager::Initialize()
{
    for (auto& obj : objects_)
    {
        obj->Initialize();
    }
}

void GameObjectManager::Update()
{
    if (isSortNeeded_)
    {
        std::sort(objects_.begin(), objects_.end(),
            [](const std::unique_ptr<GameObject>& a, const std::unique_ptr<GameObject>& b)
            {
                return a->GetUpdatePriority() < b->GetUpdatePriority();
            });
        isSortNeeded_ = false;
    }

    for (size_t i = 0; i < objects_.size(); ++i)
    {
        if (!objects_[i]->IsDead())
        {
            objects_[i]->Update();
        }
    }

    // 削除処理
    auto it = std::remove_if(objects_.begin(), objects_.end(),
        [](const std::unique_ptr<GameObject>& obj)
        {
            return obj->IsDead();
        });

    if (it != objects_.end())
    {
        objects_.erase(it, objects_.end());
    }
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

void GameObjectManager::AddObject(std::unique_ptr<GameObject> obj)
{
    objects_.push_back(std::move(obj));
    isSortNeeded_ = true;
}

void GameObjectManager::AddAllCollidersToManager(CollisionManager* manager)
{
    // 自分が持っている全てのオブジェクトをループ
    for (const auto& object : objects_)
    {
        // GameObject*をCollider*に動的キャスト
        Collider* collider = dynamic_cast<Collider*>(object.get());

        // キャストが成功し、Colliderであれば登録
        if (collider)
        {
            manager->AddCollider(collider);
        }
    }
}

GameObject* GameObjectManager::FindObjectWithTag(const std::string& tag)
{
    for (auto& obj : objects_)
    {
        if (!obj->IsDead() && obj->CompareTag(tag))
        {
            return obj.get();
        }
    }
    return nullptr;
}

std::vector<GameObject*> GameObjectManager::FindObjectsWithTag(const std::string& tag)
{
    std::vector<GameObject*> result;
    for (auto& obj : objects_)
    {
        // 生きていて、かつタグが一致するものをリストに追加
        if (!obj->IsDead() && obj->CompareTag(tag))
        {
            result.push_back(obj.get());
        }
    }
    return result;
}