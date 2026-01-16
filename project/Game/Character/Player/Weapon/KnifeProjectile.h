#pragma once
#include "Model.h" 
#include "Engine.h"
#include "Camera.h"  
#include "Vector3.h" 
#include "Collider.h"
#include "GameObject.h"

class KnifeProjectile : public GameObject, public Collider
{
public:
    KnifeProjectile(Engine* engine, Camera* camera, const Vector3& startPos, const Vector3& direction, const Vector3& collisionSize);
    ~KnifeProjectile();

    GameObjectType GetType() const override { return GameObjectType::PlayerWeapon; }

    void Update(float deltaTime);
    void Draw(); 
    void UpdateAABB(); 
    Vector3 GetWorldPosition() const override;

    // 当たり判定
    void OnCollisionStay(Collider* other) override; 
    bool IsDead() const { return lifetime_ <= 0.0f || isHit_; } 

    // 武器からの設定
    void SetDamage(float damage) { damage_ = damage; }
    void SetSpeed(float speed) { speed_ = speed; }
    void SetLifetime(float lifetime) { lifetime_ = lifetime; }

private:
    std::unique_ptr<Model> model_;
    Vector3 direction_;
    AABB aabb_;

    float speed_ = 10.0f;    
    float lifetime_ = 2.0f;  
    float damage_ = 0.0f;    
    bool isHit_ = false;
    Vector3 collisionSize_;
};