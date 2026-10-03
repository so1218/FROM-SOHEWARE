#pragma once
#include "Camera.h"
#include "PropertyBinder.h"
#include "ParticleEmitter.h"
#include "Model.h"
#include "Collider.h"

class PlayerWeapon
{
public:
    struct Config
    {
        FE::Vector4 muzzleFlashColor = { 1.0f, 0.75f, 0.3f, 1.0f };
        float muzzleFlashIntensity = 25.0f;
        float muzzleFlashRadius = 8.0f;
        float muzzleFlashDuration = 0.05f;
        FE::Vector3 muzzleOffset = { 0.0f, 0.05f, 0.35f };

        int baseDamage = 20;
        float maxDistance = 150.0f;
    };

    PlayerWeapon(FE::Engine* engine);
    ~PlayerWeapon();

    void Initialize();
    void Update(const FE::Matrix4x4& handWorldMatrix, FE::Camera* camera);
    void Draw();
    void DebugDraw(); 

    bool Fire(FE::Camera* camera, float focusRatio, float maxDamageMultiplier, float maxBulletSpread, FE::CollisionManager* colManager);
    FE::Vector3 GetMuzzleWorldPosition() const;

private:
    FE::Engine* engine_ = nullptr;
    std::unique_ptr<FE::Model> model_;
    std::unique_ptr<FE::PropertyBinder> binder_; 
    FE::ParticleEmitter* muzzleFlashEmitterPtr_ = nullptr;
    FE::ParticleEmitter* shotSmokeEmitterPtr_ = nullptr;
    FE::ParticleEmitter* shotSparkEmitterPtr_ = nullptr;
    FE::ParticleEmitter* bulletTracerEmitterPtr_ = nullptr;

    FE::WorldTransform handTransform_;

    Config config_;
    FE::Matrix4x4 currentHandMatrix_;
    int muzzleLightIndex_ = -1;
    float muzzleFlashTimer_ = 0.0f;
};