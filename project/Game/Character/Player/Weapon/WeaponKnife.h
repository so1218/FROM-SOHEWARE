#pragma once
#include "Weapon.h"
#include "KnifeProjectile.h"

class WeaponKnife : public Weapon
{
public:
    WeaponKnife(Engine* engine, Player* owner);

    void Initialize();             
    void Update(float deltaTime) override;
    void Draw() override;
    void ApplyGlobalVariables();   
    void DebugDraw();
    void LevelUp() override;

    std::vector<std::string> GetGlobalVariableGroupName() { return { "WeaponKnife" }; }

private:
    void Fire(); // 攻撃（投げる）処理

    // このナイフが発射した、すべてのアクティブな弾を管理するリスト
    std::vector<std::unique_ptr<KnifeProjectile>> projectiles_;

    float projectileSpeed_ = 20.0f;
    float projectileLifetime_ = 3.0f;
};