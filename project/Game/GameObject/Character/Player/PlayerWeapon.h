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

        int magazineSize = 10;
        float fireInterval = 0.35f;   // 1 発撃ってから次を撃てるまで
        float reloadTime = 1.6f;
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

    // 撃てる状態か (リロード中・クールタイム中・弾切れでない)
    bool CanFire() const { return !IsReloading() && fireCooldown_ <= 0.0f && ammoInMagazine_ > 0; }
    // 弾が満タンでなく、リロード中でもなければリロードを始める
    void StartReload();
    void Refill();

    int GetAmmo() const { return ammoInMagazine_; }
    int GetMagazineSize() const { return config_.magazineSize; }
    bool IsReloading() const { return reloadTimer_ > 0.0f; }
    // 0 = 始めたところ, 1 = 終わり
    float GetReloadProgress() const { return 1.0f - reloadTimer_ / config_.reloadTime; }
    int GetReloadCount() const { return reloadCount_; }

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

    int ammoInMagazine_ = 10;
    float fireCooldown_ = 0.0f;
    float reloadTimer_ = 0.0f;   // 0 より大きい間はリロード中
    int reloadCount_ = 0;        // リロードを終えた回数 (チュートリアルの判定用)
};