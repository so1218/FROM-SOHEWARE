#include "pch.h"
#include "GameObject.h"
#include "GameObjectManager.h"
#include "Collider.h"

namespace FS
{

GameObject::GameObject() = default;
GameObject::~GameObject() = default;

void GameObject::SetUpdatePriority(int priority)
{
    updatePriority_ = priority;
    if (manager_) 
    {
        manager_->RequestSort();
    }   
}

Collider* GameObject::AddCollider()
{
    auto collider = std::make_unique<Collider>(this);
    Collider* ptr = collider.get();
    colliders_.push_back(std::move(collider));

    // すでにマネージャーに登録済みなら、すぐ登録
    if (manager_)
    {
        ptr->RegisterToManager();
    }
    return ptr;
}

void GameObject::RegisterColliders()
{
    for (auto& collider : colliders_)
    {
        collider->RegisterToManager();   // 登録済みなら何もしない 
    }
}

void GameObject::AttachToManager(GameObjectManager* manager, uint64_t id)
{
    // 途中でマネージャーや ID を書き換えられないようにする
    assert(manager_ == nullptr && "GameObject は 1 つのマネージャーにしか追加できない");
    manager_ = manager;
    id_ = id;

    // コンストラクタで AddCollider() されたコライダーをここで登録する。
    // GameObjectManager の管理外 (ID 0) で、他のオブジェクトが所有する場合もこれで登録漏れがなくなる
    RegisterColliders();
}

void GameObject::RemoveCollider(Collider* collider)
{
    std::erase_if(colliders_, [collider](const std::unique_ptr<Collider>& c) { return c.get() == collider; });
}

}