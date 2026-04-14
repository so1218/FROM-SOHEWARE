#pragma once
#include "Engine.h"
#include "ModelManager.h"
#include "AnimationManager.h"
#include "TextureManager.h"
#include "Sprite.h"
#include "Model.h"
#include "AnimationModel.h"

namespace FE
{ 

class Collider;
class GameObjectManager;

class GameObject 
{
public:
    GameObject();
    virtual ~GameObject() = default;

    virtual void Initialize() {}
    virtual void Update() {}
    virtual void Draw() {}
    virtual void DebugDraw() {}

    // 生存フラグ
    bool IsDead() const { return isDead_; }
    void Destroy() { isDead_ = true; }

    // タグ
    void SetTag(uint32_t tag) { tag_ = tag; }
    uint32_t GetTag() const { return tag_; }
    bool CompareTag(uint32_t tag) const { return tag_ == tag; }

    // 優先度
    void SetUpdatePriority(int priority) { updatePriority_ = priority; }
    int GetUpdatePriority() const { return updatePriority_; }

    // Transform
    WorldTransform& GetTransform() { return transform_; }
    const WorldTransform& GetTransform() const { return transform_; }

    // 衝突コールバック
    virtual void OnCollisionEnter(Collider* mine, Collider* other) {}
    virtual void OnCollisionStay(Collider* mine, Collider* other) {}
    virtual void OnCollisionExit(Collider* mine, Collider* other) {}

    // マネージャー
    void SetManager(GameObjectManager* manager) { manager_ = manager; }
    GameObjectManager* GetManager() const { return manager_; }

private:
    WorldTransform transform_;
    uint32_t tag_ = 0;
    int updatePriority_ = 50;
    bool isDead_ = false;
    GameObjectManager* manager_ = nullptr;
};

}