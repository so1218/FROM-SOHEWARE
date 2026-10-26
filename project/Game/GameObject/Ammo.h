#pragma once
#include "GameObject.h"
#include "Model.h"
#include "Collider.h"
#include "PropertyBinder.h"
#include "ParticleEmitter.h"

class Ammo : public FS::GameObject
{
public:
    Ammo(FS::Engine* engine, int id, const std::string& parentGroupName);
    ~Ammo() override;

    void Initialize() override;
    void Update(const FS::Vector3& scale, const FS::Vector3& bubbleScale, const FS::Vector4& lightColor, float tiltAngle, float rotationSpeed);
    void Draw() override;
    void DebugDraw() override;

    void Sleep();
    bool IsPicked() const { return isPicked_; }

    void OnCollisionStay(FS::Collider* mine, FS::Collider* other) override;
    FS::Model* GetModel() const { return model_.get(); }
    FS::Model* GetBubbleModel() const { return bubbleModel_.get(); }

private:
    FS::Engine* engine_ = nullptr;
    std::unique_ptr<FS::Model> model_;
    std::unique_ptr<FS::Model> bubbleModel_;
    std::unique_ptr<FS::Collider> collider_;
    std::unique_ptr<FS::PropertyBinder> binder_;

    std::unique_ptr<FS::ParticleEmitter> hitEmitter_ = nullptr;
    FS::ParticleEmitter* hitEmitterPtr_ = nullptr;

    int id_;
    bool isPicked_ = false;

    float lightIntensity_ = 5.0f;
    float lightRadius_ = 10.0f;
    float lightVolumetricScatteringIntensity_ = 1.0f;

    // 自転用の累積回転角度
    float rotationAngle_ = 0.0f;
};
