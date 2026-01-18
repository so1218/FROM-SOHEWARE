#pragma once
#include "Weapon.h"
#include "KnifeProjectile.h"

class WeaponKnife : public Weapon
{
public:
    WeaponKnife(Engine* engine, Player* player);

    void Initialize();             
    void Update(float deltaTime) override;
    void Draw() override;
    void ApplyGlobalVariables();   
    void DebugDraw();
    void LevelUp() override;
    void AddCollidersToManager(CollisionManager* manager) override;
    WeaponType GetType() const override { return WeaponType::Knife; }

    std::vector<std::string> GetGlobalVariableGroupName() { return { "WeaponKnife" }; }

private:
    // このナイフが発射した、すべてのアクティブな弾を管理するリスト
    std::vector<std::unique_ptr<KnifeProjectile>> projectiles_;

    Camera* camera_ = nullptr;

    float projectileSpeed_ = 20.0f;
    float projectileLifetime_ = 3.0f;

    // 連射制御用のメンバ変数
    int projectilesToFire_ = 0;     // このバーストで発射する残り弾数
    float burstTimer_ = 0.0f;       // 連射間隔タイマー
    float timeBetweenProjectiles_ = 0.1f;

    // レベルアップ効果を適用するヘルパー関数
    void ApplyLevelEffects() override;

    void FireOneProjectile(); // 攻撃処理
};