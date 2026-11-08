#include "pch.h"
#include "GameObject.h"
#include "GameObjectManager.h"
#include "Collider.h"
#include "CollisionManager.h"

namespace FS
{

GameObject::GameObject() = default;

GameObject::~GameObject()
{
    if (collisionManager_)
    {
        for (const auto& collider : colliders_)
        {
            collisionManager_->RemoveCollider(collider.get());
        }
    }
}

void GameObject::SetUpdatePriority(int priority)
{
    updatePriority_ = priority;
    if (manager_) 
    {
        manager_->RequestSort();
    }   
}

Collider* GameObject::AddSphereCollider(float radius)
{
    auto collider = std::make_unique<Collider>(this, CollisionShapeType::Sphere);
    collider->SetRadius(radius);
    return AddColliderWithShape(std::move(collider));
}

Collider* GameObject::AddBoxCollider(const Vector3& size)
{
    auto collider = std::make_unique<Collider>(this, CollisionShapeType::Box);
    collider->SetSize(size);
    return AddColliderWithShape(std::move(collider));
}

Collider* GameObject::AddColliderWithShape(std::unique_ptr<Collider> collider)
{
    Collider* ptr = collider.get();
    colliders_.push_back(std::move(collider));

    // マネージャーに追加される前なら、追加されたときに登録する
    if (collisionManager_)
    {
        collisionManager_->AddCollider(ptr);
    }
    return ptr;
}

void GameObject::AttachToManager(GameObjectManager* manager, uint64_t id)
{
    // 途中でマネージャーや ID を書き換えられないようにする
    assert(manager_ == nullptr && "GameObject は 1 つのマネージャーにしか追加できない");
    manager_ = manager;
    id_ = id;

    // コンストラクタで付けたコライダーを登録
    collisionManager_ = manager->GetCollisionManager();
    if (collisionManager_)
    {
        for (const auto& collider : colliders_)
        {
            collisionManager_->AddCollider(collider.get());
        }
    }
}

void GameObject::RemoveCollider(Collider* collider)
{
    if (collisionManager_)
    {
        collisionManager_->RemoveCollider(collider);
    }
    std::erase_if(colliders_, [collider](const std::unique_ptr<Collider>& c) { return c.get() == collider; });
}

}
