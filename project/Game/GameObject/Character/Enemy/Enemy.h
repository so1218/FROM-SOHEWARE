#pragma once
#include "GameObject.h"
#include "Model.h"
#include "Collider.h"
#include "PropertyBinder.h"
#include "ParticleEmitter.h"

class Enemy : public FE::GameObject
{
public:
    Enemy(FE::Engine* engine, int id, const std::string& parentGroupName);
    ~Enemy() override;

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugDraw() override;

    void OnCollisionEnter(FE::Collider* mine, FE::Collider* other) override;

    FE::Model* GetModel() const { return model_.get(); }

private:
    FE::Engine* engine_;
    std::unique_ptr<FE::Model> model_;
    std::unique_ptr<FE::Collider> collider_;
    std::unique_ptr<FE::PropertyBinder> binder_;

    std::unique_ptr<FE::ParticleEmitter> auraEmitter_ = nullptr;

    int id_;

    // 波の動き用パラメーター
    FE::Vector3 basePosition_; // 基準となる位置
    FE::Vector3 amplitude_;    // 振幅
    FE::Vector3 frequency_;    // 周波数
    float time_ = 0.0f;        // 経過時間
    FE::Vector3 phase_;

    float colliderRadius_ = 1.0f;
    FE::Vector3 colliderOffset_ = { 0.0f, 0.0f, 0.0f };

    int spotLightIndex_ = -1;
    FE::Vector4 spotColor_ = { 1.0f, 1.0f, 0.8f, 1.0f }; 
    float spotIntensity_ = 8.0f;
    float spotDistance_ = 20.0f;
    float spotAngleDeg_ = 30.0f; 
    float spotVolumetric_ = 4.0f;
    FE::Vector3 spotDirection_ = { 0.0f, -1.0f, 0.0f };
};