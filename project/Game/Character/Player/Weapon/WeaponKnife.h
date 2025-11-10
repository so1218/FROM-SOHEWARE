#pragma once
#include "Weapon.h"
#include "KnifeProjectile.h"

class WeaponKnife : public Weapon
{
public:
    WeaponKnife(Engine* engine, Player* player, Camera* camera);

    void Initialize();             
    void Update(float deltaTime) override;
    void Draw() override;
    void ApplyGlobalVariables();   
    void DebugDraw();
    void LevelUp() override;
    void AddCollidersToManager(CollisionManager* manager) override;

    std::vector<std::string> GetGlobalVariableGroupName() { return { "WeaponKnife" }; }

private:
    void Fire(); // 攻撃処理

    // このナイフが発射した、すべてのアクティブな弾を管理するリスト
    std::vector<std::unique_ptr<KnifeProjectile>> projectiles_;

    Camera* camera_ = nullptr;

    float projectileSpeed_ = 20.0f;
    float projectileLifetime_ = 3.0f;
};