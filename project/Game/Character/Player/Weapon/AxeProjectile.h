#pragma once
#include "GameObject.h" 
#include "Collider.h"
#include "Model.h"
#include "Camera.h"
#include "Engine.h"
#include "Enemy.h"

class AxeProjectile : public GameObject, public Collider
{
public:
    AxeProjectile(Engine* engine, const Vector3& startPos, const Vector3& initialVelocity, float initialYaw);
    ~AxeProjectile();

    GameObjectType GetType() const override { return GameObjectType::PlayerWeapon; }

    void Update() override;
    void Draw() override;
    void UpdateAABB();
    Vector3 GetWorldPosition() const override;

    // 当たり判定
    void OnCollisionStay(Collider* other) override;
    bool IsDead() const { return lifetime_ <= 0.0f || isHit_; }

    // 武器からの設定
    void SetDamage(float damage) { damage_ = damage; }
    void SetLifetime(float lifetime) { lifetime_ = lifetime; }

    // サイズをセットする関数
    void SetSize(const Vector3& size);

private:
    std::unique_ptr<Model> model_;
    AABB aabb_;

    Vector3 velocity_;      
    float gravity_ = -15.0f;

    float lifetime_ = 3.0f;
    float damage_ = 0.0f;
    bool isHit_ = false;
};