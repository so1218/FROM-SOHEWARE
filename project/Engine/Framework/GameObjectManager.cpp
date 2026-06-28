#include "pch.h"
#include "GameObjectManager.h"
#include "CollisionManager.h" 
#include "Collider.h"

namespace FE
{

void GameObjectManager::Initialize()
{
    for (auto& obj : objects_)
    {
        obj->Initialize();
    }
}

void GameObjectManager::Update()
{
    isUpdating_ = true;

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
        if (!objects_[i]->IsDead() && objects_[i]->IsActive())
        {
            objects_[i]->Update();
        }
    }

    // 行列更新
    for (size_t i = 0; i < objects_.size(); ++i)
    {
        if (!objects_[i]->IsDead() && objects_[i]->IsActive())
        {
            objects_[i]->GetTransform().UpdateMatrix();
        }
    }

    isUpdating_ = false;

    // 待機オブジェクトの合流
    for (auto& newObj : pendingObjects_) {
        newObj->Initialize();
        objects_.push_back(std::move(newObj));
        isSortNeeded_ = true;
    }
    pendingObjects_.clear();

    // 削除処理
    auto it = std::remove_if(objects_.begin(), objects_.end(),
        [](const std::unique_ptr<GameObject>& obj) { return obj->IsDead(); });

    if (it != objects_.end())
    {
        objects_.erase(it, objects_.end());
    }
}

void GameObjectManager::Draw()
{
    for (auto& obj : objects_)
    {
        if (obj->IsActive() && !obj->IsDead())
        {
            obj->Draw();
        }
    }
}

void GameObjectManager::DebugDraw()
{
    for (auto& obj : objects_)
    {
        if (obj->IsActive() && !obj->IsDead())
        {
            obj->DebugDraw();
        }
    }
}

void GameObjectManager::AddObject(std::unique_ptr<GameObject> obj)
{
    obj->SetManager(this);
    if (isUpdating_)
    {
        pendingObjects_.push_back(std::move(obj)); // ループ中は待機列へ
    }
    else 
    {
        objects_.push_back(std::move(obj)); // それ以外は直接追加
        isSortNeeded_ = true;
    }
}

GameObject* GameObjectManager::FindObjectWithTag(uint32_t tag)
{
    for (auto& obj : objects_)
    {
        if (!obj->IsDead() && obj->IsActive() && obj->CompareTag(tag))
        {
            return obj.get();
        }
    }
    return nullptr;
}

std::vector<GameObject*> GameObjectManager::FindObjectsWithTag(uint32_t tag)
{
    std::vector<GameObject*> result;
    for (auto& obj : objects_)
    {
        // 生きていて、かつタグが一致するものをリストに追加
        if (!obj->IsDead() && obj->IsActive() && obj->CompareTag(tag))
        {
            result.push_back(obj.get());
        }
    }
    return result;
}

}