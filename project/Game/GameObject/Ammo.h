#pragma once
#include "GameObject.h"
#include "Model.h"
#include "Collider.h"
#include "PropertyBinder.h"
#include "ParticleEmitter.h"

class Ammo : public FE::GameObject
{
public:
    Ammo(FE::Engine* engine, int id, const std::string& parentGroupName);
    ~Ammo() override;

    void Initialize() override;
    void Update(const FE::Vector3& scale, const FE::Vector3& bubbleScale, const FE::Vector4& lightColor, float tiltAngle, float rotationSpeed);
    void Draw() override;
    void DebugDraw() override;

    void Sleep();
    bool IsPicked() const { return isPicked_; }

    void OnCollisionStay(FE::Collider* mine, FE::Collider* other) override;
    FE::Model* GetModel() const { return model_.get(); }
    FE::Model* GetBubbleModel() const { return bubbleModel_.get(); }

private:
    FE::Engine* engine_ = nullptr;
    std::unique_ptr<FE::Model> model_;
    std::unique_ptr<FE::Model> bubbleModel_;
    std::unique_ptr<FE::Collider> collider_;
    std::unique_ptr<FE::PropertyBinder> binder_;

    std::unique_ptr<FE::ParticleEmitter> hitEmitter_ = nullptr;
    FE::ParticleEmitter* hitEmitterPtr_ = nullptr;

    int id_;
    bool isPicked_ = false;

    int pointLightIndex_ = -1;
    float lightIntensity_ = 5.0f;
    float lightRadius_ = 10.0f;
    float lightVolumetricScatteringIntensity_ = 1.0f;

    // 自転用の累積回転角度
    float rotationAngle_ = 0.0f;
};
