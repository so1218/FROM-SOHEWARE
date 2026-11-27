#pragma once
#include "Camera.h"
#include "PropertyBinder.h"
#include "ParticleEmitter.h"
#include "Model.h"

namespace FS
{
class CollisionManager;
class Terrain;
}

class TreeField;

class PlayerWeapon
{
public:
    // 弾の種類
    enum class AmmoType
    {
        Normal,
        Incendiary,  // 当たった所の草に火をつける
    };

    // 銃から飛び出す物の出方
    struct EjectSettings
    {
        FS::Vector3 offset = { 0.0f, 0.0f, 0.0f };
        FS::Vector3 direction = { 0.0f, 1.0f, 0.0f };
        FS::Vector3 scale = { 0.1f, 0.1f, 0.1f };
        float speed = 3.0f;
        float spread = 0.3f;        
        float spinSpeed = 7.0f;    
        float gravity = 30.0f;
        float lifeTime = 1.0f;     
        float shrinkTime = 0.2f;  
    };

    struct Config
    {
        FS::Vector4 muzzleFlashColor = { 1.0f, 0.75f, 0.3f, 1.0f };
        float muzzleFlashIntensity = 25.0f;
        float muzzleFlashRadius = 8.0f;
        float muzzleFlashDuration = 0.05f;
        float muzzleFlashVolumetricIntensity = 1.0f;
        FS::Vector3 muzzleOffset = { 0.0f, 0.05f, 0.35f };

        // 銃声
        float shotSoundVolume = 0.2f;        
        float shotBoomVolume = 0.8f;        
        float shotTrailVolume = 0.15f;       
        float shotIncendiaryVolume = 0.6f;  

        int baseDamage = 20;
        float maxDistance = 150.0f;

        // 焼夷弾
        float incendiaryRadius = 2.0f;        
        float incendiaryGroundReach = 4.0f;  

        int magazineSize = 10;
        int incendiaryMagazineSize = 2;   
        float fireInterval = 0.35f;   
        float reloadTime = 1.6f;

        // リロードの演出
        FS::Vector3 reloadTiltAxis = { 1.0f, 0.0f, 0.0f };  
        float reloadTiltAngle = 35.0f;     
        float reloadTiltInTime = 0.15f;    
        float reloadTiltOutTime = 0.15f;    

        EjectSettings magazine = { .direction = { 0.7f, 1.0f, 0.0f }, .scale = { 0.1f, 0.25f, 0.15f } };
        EjectSettings casing = {
            .offset = { -1.0f, 1.4f, 0.45f },
            .direction = { 1.0f, 1.0f, -0.3f },
            .scale = { 0.04f, 0.04f, 0.04f },
            .speed = 4.0f,
            .spread = 0.25f,
            .spinSpeed = 15.0f,
            .lifeTime = 0.6f,
        };
    };

    PlayerWeapon(FS::Engine* engine);
    ~PlayerWeapon();

    void Initialize();
    void Update(const FS::Matrix4x4& handWorldMatrix, FS::Camera* camera);
    void Draw();
    void DebugGui(); 

    // terrain を渡すと、弾が地面で止まる
    bool Fire(FS::Camera* camera, float focusRatio, float maxDamageMultiplier, float maxBulletSpread, FS::CollisionManager* colManager,
        TreeField* treeField, const FS::Terrain* terrain);

    void ToggleAmmoType();
    AmmoType GetAmmoType() const { return ammoType_; }
    FS::Vector3 GetMuzzleWorldPosition() const;

    // 撃てる状態か 
    bool CanFire() const { return !IsReloading() && fireCooldown_ <= 0.0f && GetAmmo() > 0; }
    // 弾が満タンでなく、リロード中でもなければリロードを始める
    void StartReload();
    // リロードを途中でやめる
    void CancelReload() { reloadTimer_ = 0.0f; }
    void Refill();
    // 弾の種類を普通の弾に戻す
    void ResetAmmoType() { ammoType_ = AmmoType::Normal; }
    // 今の弾の種類の弾倉
    int& GetCurrentAmmo() { return ammoType_ == AmmoType::Incendiary ? incendiaryAmmoInMagazine_ : ammoInMagazine_; }

    // 今の弾の種類の残弾と装弾数
    int GetAmmo() const { return ammoType_ == AmmoType::Incendiary ? incendiaryAmmoInMagazine_ : ammoInMagazine_; }
    int GetMagazineSize() const { return ammoType_ == AmmoType::Incendiary ? config_.incendiaryMagazineSize : config_.magazineSize; }
    bool IsReloading() const { return reloadTimer_ > 0.0f; }
    float GetReloadProgress() const { return 1.0f - reloadTimer_ / config_.reloadTime; }
    int GetReloadCount() const { return reloadCount_; }
    // 焼夷弾で草に火をつけた回数
    int GetIgniteCount() const { return igniteCount_; }


private:
    // 銃から飛び出した物
    struct EjectedObject
    {
        std::unique_ptr<FS::Model> model;
        const EjectSettings* settings = nullptr;   
        FS::Vector3 velocity;
        FS::Vector3 spinAxis;
        float spinSpeed = 0.0f;
        float elapsedTime = 0.0f;
    };

    float CalculateReloadTilt() const;
    // 地面に当たる所を探す。当たらなければ false
    static bool RaycastTerrain(const FS::Terrain* terrain, const FS::Vector3& start, const FS::Vector3& direction, float maxDistance, FS::Vector3& outPoint);
    // 焼夷弾が当たった所の真下の草に火をつける
    void IgniteAt(const FS::Terrain* terrain, const FS::Vector3& hitPoint);
    void DropMagazine();
    void Eject(const EjectSettings& settings, const std::string& modelName, const FS::Model* materialSource);
    void UpdateEjectedObject(EjectedObject& ejectedObject, float deltaTime);
    void BindEjectSettings(const std::string& prefix, EjectSettings& settings);
    void DrawEjectSettings(const std::string& prefix);

    FS::Engine* engine_ = nullptr;
    FS::Camera* camera_ = nullptr;   
    std::unique_ptr<FS::Model> model_;
    std::unique_ptr<FS::PropertyBinder> binder_; 
    FS::ParticleEmitter* muzzleFlashEmitterPtr_ = nullptr;
    FS::ParticleEmitter* shotSmokeEmitterPtr_ = nullptr;
    FS::ParticleEmitter* shotSparkEmitterPtr_ = nullptr;
    FS::ParticleEmitter* bulletTracerEmitterPtr_ = nullptr;
    FS::ParticleEmitter* incendiaryTracerEmitterPtr_ = nullptr;
    FS::ParticleEmitter* incendiaryMuzzleBurstEmitterPtr_ = nullptr;
    FS::ParticleEmitter* woodHitBulletEmitterPtr_ = nullptr;

    FS::WorldTransform handTransform_;
    FS::WorldTransform reloadTiltTransform_;   

    // 落とすマガジンのマテリアルの共有元
    std::unique_ptr<FS::Model> magazineModel_;
    std::unique_ptr<FS::Model> casingModel_;   
    std::vector<EjectedObject> ejectedObjects_;

    Config config_;
    FS::Matrix4x4 currentHandMatrix_;
    float muzzleFlashTimer_ = 0.0f;

    int ammoInMagazine_ = 10;
    int incendiaryAmmoInMagazine_ = 2;
    AmmoType ammoType_ = AmmoType::Normal;
    float fireCooldown_ = 0.0f;
    float reloadTimer_ = 0.0f;   
    int reloadCount_ = 0;      
    int igniteCount_ = 0;
};