#pragma once
#include "Camera.h"
#include "PropertyBinder.h"
#include "ParticleEmitter.h"
#include "Model.h"
#include "Collider.h"
#include <random>

class TreeField;

class PlayerWeapon
{
public:
    // 銃から飛び出す物 (マガジン、薬莢) の出方。位置は銃から見た値、向きはカメラから見た値 (x = 右, y = 上, z = 奥)、
    // 大きさはワールドでの大きさ
    struct EjectSettings
    {
        FS::Vector3 offset = { 0.0f, 0.0f, 0.0f };
        FS::Vector3 direction = { 0.0f, 1.0f, 0.0f };
        FS::Vector3 scale = { 0.1f, 0.1f, 0.1f };
        float speed = 3.0f;
        float spread = 0.3f;        // 向きのばらつき
        float spinSpeed = 7.0f;     // 回る速さ (ラジアン / 秒)。毎回少し変える
        float gravity = 30.0f;
        float lifeTime = 1.0f;      // 出てから消えるまで (秒)
    };

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

        // リロードの演出。アニメーションの代わりに、銃を手の中で傾けてマガジンを落とす
        FS::Vector3 reloadTiltAxis = { 1.0f, 0.0f, 0.0f };   // 手から見た軸
        float reloadTiltAngle = 35.0f;      // 度
        float reloadTiltInTime = 0.15f;     // 傾けるまで (秒)
        float reloadTiltOutTime = 0.15f;    // 戻すまで (秒)

        EjectSettings magazine = { .direction = { 0.7f, 1.0f, 0.0f }, .scale = { 0.1f, 0.25f, 0.15f } };
        // ハンドガンは右上に排莢する。少しカメラ側に飛ばすと後ろからの視点でよく見える
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
    // 銃から飛び出した物。跳ね上がってから落ち、決まった時間で消える
    struct EjectedObject
    {
        std::unique_ptr<FS::Model> model;
        const EjectSettings* settings = nullptr;   // 調整中の変更がすぐ反映されるよう、Config を指す
        FS::Vector3 velocity;
        FS::Vector3 spinAxis;
        float spinSpeed = 0.0f;
        float elapsedTime = 0.0f;
    };

    float CalculateReloadTilt() const;
    void DropMagazine();
    void Eject(const EjectSettings& settings, const std::string& modelName, const FS::Model* materialSource);
    void UpdateEjectedObject(EjectedObject& ejectedObject, float deltaTime);
    void BindEjectSettings(const std::string& prefix, EjectSettings& settings);
    void DrawEjectSettings(const std::string& prefix);

    FS::Engine* engine_ = nullptr;
    FS::Camera* camera_ = nullptr;   // 飛び出す物の向きの基準
    std::unique_ptr<FS::Model> model_;
    std::unique_ptr<FS::PropertyBinder> binder_; 
    FS::ParticleEmitter* muzzleFlashEmitterPtr_ = nullptr;
    FS::ParticleEmitter* shotSmokeEmitterPtr_ = nullptr;
    FS::ParticleEmitter* shotSparkEmitterPtr_ = nullptr;
    FS::ParticleEmitter* bulletTracerEmitterPtr_ = nullptr;
    FS::ParticleEmitter* woodHitBulletEmitterPtr_ = nullptr;

    FS::WorldTransform handTransform_;
    FS::WorldTransform reloadTiltTransform_;   // 手と銃の間に挟み、リロード中だけ回す

    // 落とすマガジンのマテリアルの共有元。ImGui で色や質感を編集する (位置や大きさは EjectSettings で決める)
    std::unique_ptr<FS::Model> magazineModel_;
    std::unique_ptr<FS::Model> casingModel_;   // 薬莢のマテリアルの共有元
    std::vector<EjectedObject> ejectedObjects_;
    std::mt19937 random_;

    Config config_;
    FS::Matrix4x4 currentHandMatrix_;
    float muzzleFlashTimer_ = 0.0f;

    int ammoInMagazine_ = 10;
    float fireCooldown_ = 0.0f;
    float reloadTimer_ = 0.0f;   // 0 より大きい間はリロード中
    int reloadCount_ = 0;        // リロードを終えた回数 (チュートリアルの判定用)
};