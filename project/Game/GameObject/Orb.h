#pragma once
#include "GameObject.h"
#include "Model.h"
#include "Collider.h"
#include "PropertyBinder.h"
#include "ParticleEmitter.h"

class Orb : public FE::GameObject
{
public:
    Orb(FE::Engine* engine, int id, const std::string& parentGroupName);
    ~Orb() override;

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugDraw() override;

    void Sleep(); 
    bool IsPicked() const { return isPicked_; }

    void OnCollisionStay(FE::Collider* mine, FE::Collider* other) override;

private:
    FE::Engine* engine_;
    std::unique_ptr<FE::Model> model_;
    std::unique_ptr<FE::Collider> collider_;
    std::unique_ptr<FE::PropertyBinder> binder_;

    std::unique_ptr<FE::ParticleEmitter> hitEmitter_ = nullptr;
    FE::ParticleEmitter* hitEmitterPtr_ = nullptr;

    int id_;
    bool isPicked_ = false; // 拾われたかどうかのフラグ

    // ポイントライト設定
    int pointLightIndex_ = -1;
    FE::Vector4 lightColor_ = { 0.2f, 0.6f, 1.0f, 1.0f };
    float lightIntensity_ = 5.0f;
    float lightRadius_ = 10.0f;
    float lightVolumetricScatteringIntensity_ = 1.0f;
};
