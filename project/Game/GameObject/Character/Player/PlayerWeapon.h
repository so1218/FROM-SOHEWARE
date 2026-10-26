#pragma once
#include "Camera.h"
#include "PropertyBinder.h"
#include "ParticleEmitter.h"
#include "Model.h"
#include "Collider.h"

class TreeField;

class PlayerWeapon
{
public:
    struct Config
    {
        FS::Vector4 muzzleFlashColor = { 1.0f, 0.75f, 0.3f, 1.0f };
        float muzzleFlashIntensity = 25.0f;
        float muzzleFlashRadius = 8.0f;
        float muzzleFlashDuration = 0.05f;
        float muzzleFlashVolumetricIntensity = 1.0f;
        FS::Vector3 muzzleOffset = { 0.0f, 0.05f, 0.35f };

        int baseDamage = 20;
        float maxDistance = 150.0f;
    };

    PlayerWeapon(FS::Engine* engine);
    ~PlayerWeapon();

    void Initialize();
    void Update(const FS::Matrix4x4& handWorldMatrix, FS::Camera* camera);
    void Draw();
    void DebugDraw(); 

    bool Fire(FS::Camera* camera, float focusRatio, float maxDamageMultiplier, float maxBulletSpread, FS::CollisionManager* colManager,
        TreeField* treeField);
    FS::Vector3 GetMuzzleWorldPosition() const;

private:
    FS::Engine* engine_ = nullptr;
    std::unique_ptr<FS::Model> model_;
    std::unique_ptr<FS::PropertyBinder> binder_; 
    FS::ParticleEmitter* muzzleFlashEmitterPtr_ = nullptr;
    FS::ParticleEmitter* shotSmokeEmitterPtr_ = nullptr;
    FS::ParticleEmitter* shotSparkEmitterPtr_ = nullptr;
    FS::ParticleEmitter* bulletTracerEmitterPtr_ = nullptr;
    FS::ParticleEmitter* woodHitBulletEmitterPtr_ = nullptr;

    FS::WorldTransform handTransform_;

    Config config_;
    FS::Matrix4x4 currentHandMatrix_;
    float muzzleFlashTimer_ = 0.0f;
};