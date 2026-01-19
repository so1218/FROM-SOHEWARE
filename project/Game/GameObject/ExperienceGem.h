#pragma once

#include "Collider.h"
#include "BaseCharacter.h"
#include "Model.h"
#include "Camera.h"
#include "Engine.h"

class Player; 

class ExperienceGem : public Collider, public GameObject
{
public:
    ExperienceGem(Engine* engine, Player* player);

    GameObjectType GetType() const override { return GameObjectType::Enemy; } 

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void OnCollisionStay(Collider* other) override;

    Vector3 GetWorldPosition() const override;
    WorldTransform& GetWorldTransform() { return model_->GetTransform(); }

    // 収集されたか
    bool IsCollected() const { return isCollected_; }

    bool IsDead() const override { return isCollected_; }

private:
    Player* player_;
    std::unique_ptr<Model> model_;

    bool isCollected_ = false;

    float magnetRadius_ = 7.0f; // 引き寄せられる範囲
    float moveSpeed_ = 15.0f;   // 引き寄せられる速度
};