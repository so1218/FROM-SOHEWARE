#include "pch.h"
#include "GameObjectManager.h"
#include "CollisionManager.h" 
#include "Collider.h"

namespace FS
{

void GameObjectManager::Initialize()
{
    isInitialized_ = true;

    const size_t count = objects_.size();
    for (size_t i = 0; i < count; ++i)
    {
        SetupNewObject(objects_[i].get());
    }
}

void GameObjectManager::SetupNewObject(GameObject* obj)
{
    obj->Initialize();
    obj->GetTransform().UpdateMatrix();   // 追加直後のフレームに原点で描画・判定されるのを防ぐ
    obj->RegisterColliders();             // AddCollider() したコライダーを自動登録
}


void GameObjectManager::Update()
{
    isUpdating_ = true;

    if (isSortNeeded_)
    {
        std::stable_sort(objects_.begin(), objects_.end(),
            [](const std::unique_ptr<GameObject>& a, const std::unique_ptr<GameObject>& b)
            {
                return a->GetUpdatePriority() < b->GetUpdatePriority();
            });
        isSortNeeded_ = false;
    }

    for (size_t i = 0; i < objects_.size(); ++i)
    {
        if (IsAlive(objects_[i].get())) objects_[i]->Update();
    }

    // 行列更新
    for (size_t i = 0; i < objects_.size(); ++i)
    {
        if (IsAlive(objects_[i].get())) objects_[i]->GetTransform().UpdateMatrix();
    }

    // 削除されるオブジェクトの OnDestroy
    for (size_t i = 0; i < objects_.size(); ++i)
    {
        if (objects_[i]->IsDead()) 
        {
            objects_[i]->OnDestroy();
        }
    }

    isUpdating_ = false;

    RemoveDeadObjects();
    MergePendingObjects();
}

void GameObjectManager::RemoveDeadObjects()
{
    auto it = std::remove_if(objects_.begin(), objects_.end(),
        [](const std::unique_ptr<GameObject>& obj) { return obj->IsDead(); });

    // ID の登録を先に消す
    for (auto i = it; i != objects_.end(); ++i)
    {
        idMap_.erase((*i)->GetId());
    }

    objects_.erase(it, objects_.end());
}

void GameObjectManager::MergePendingObjects()
{
    std::vector<std::unique_ptr<GameObject>> pending;
    pending.swap(pendingObjects_);

    for (auto& obj : pending)
    {
        // 生成された同じフレームに Destroy されたもの
        if (obj->IsDead())
        {
            obj->OnDestroy();
            idMap_.erase(obj->GetId());
            continue;   // obj はこのループの終わりで破棄
        }

        GameObject* raw = obj.get();
        objects_.push_back(std::move(obj));
        isSortNeeded_ = true;
        SetupNewObject(raw);
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
    obj->AttachToManager(this, nextId_++);
    idMap_[obj->GetId()] = obj.get();

    if (isUpdating_)
    {
        pendingObjects_.push_back(std::move(obj));   // 更新中は待機リストへ
        return;
    }

    GameObject* raw = obj.get();
    objects_.push_back(std::move(obj));
    isSortNeeded_ = true;

    if (isInitialized_)
    {
        SetupNewObject(raw);   // シーン開始後に追加されたものはすぐ初期化
    }
}

GameObject* GameObjectManager::FindObjectWithTag(uint32_t tag)
{
    for (auto& obj : objects_)
    {
        if (IsAlive(obj.get()) && obj->CompareTag(tag))
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
        if (IsAlive(obj.get()) && obj->CompareTag(tag))
        {
            result.push_back(obj.get());
        }
    }
    return result;
}


}