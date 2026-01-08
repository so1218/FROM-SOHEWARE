#pragma once
#include "Weapon.h"
#include "AxeProjectile.h" 
#include <vector>
#include <memory>

class WeaponAxe : public Weapon
{
public:
    WeaponAxe(Engine* engine, Player* player, Camera* camera);

    void Initialize();
    void Update(float deltaTime) override;
    void Draw() override;
    void ApplyGlobalVariables();
    void DebugDraw();
    void LevelUp() override;
    void ApplyLevelEffects() override;
    void AddCollidersToManager(CollisionManager* manager) override;
    WeaponType GetType() const override { return WeaponType::Axe; }

    std::vector<std::string> GetGlobalVariableGroupName() { return { "WeaponAxe" }; } 

private:
    void Fire();

    std::vector<std::unique_ptr<AxeProjectile>> projectiles_;

    Camera* camera_ = nullptr;

    // 斧の初期ジャンプ力や寿命
    float projectileInitialSpeedY_ = 10.0f; //  Y軸への初速
    float projectileLifetime_ = 3.0f;

    // ベースのサイズ
    Vector3 collisionSize_ = { 0.5f, 0.5f, 0.5f };

    Vector3 currentCollisionSize_ = { 0.5f, 0.5f, 0.5f };
};