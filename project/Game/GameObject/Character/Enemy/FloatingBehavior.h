#pragma once
#include "Engine.h"
#include "IEnemyBehavior.h"

// 浮遊する敵の挙動
class FloatingBehavior : public IEnemyBehavior
{
public:
    FloatingBehavior() = default;
    ~FloatingBehavior() override;

    void Initialize(Enemy* owner) override;
    void Update(Enemy* owner) override;
    void DebugDraw(Enemy* owner) override; 

    int GetInitialHP() const override { return 500; } 
    std::string GetDamageParticleName() const override { return "floatingDamage"; } 

    void OnTakeDamage(Enemy* owner, int damage, const FE::Vector3& hitPoint, const FE::Vector3& hitNormal) override;
    void OnDeath(Enemy* owner) override;

private:
    FE::Engine* engine_ = nullptr;
    FE::ParticleEmitter* auraEmitterPtr_ = nullptr;
    FE::ParticleEmitter* explosionEmitterPtr_ = nullptr;

    // 浮遊用パラメータ
    FE::Vector3 basePosition_;
    FE::Vector3 amplitude_ = { 3.0f, 2.0f, 0.0f };
    FE::Vector3 frequency_ = { 0.5f, 1.0f, 0.0f };
    FE::Vector3 phase_ = { 0.0f, 0.0f, 0.0f };
    float time_ = 0.0f;

    // スポットライト用パラメータ
    int spotLightIndex_ = -1;
    FE::Vector4 spotColor_ = { 1.0f, 1.0f, 0.8f, 1.0f };
    float spotIntensity_ = 8.0f;
    float spotDistance_ = 20.0f;
    float spotAngleDeg_ = 30.0f;
    float spotVolumetric_ = 4.0f;
    FE::Vector3 spotDirection_ = { 0.0f, -1.0f, 0.0f };
};