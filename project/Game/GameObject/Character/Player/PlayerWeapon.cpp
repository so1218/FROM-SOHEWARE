#include "pch.h"
#include "PlayerWeapon.h"
#include "Engine.h"
#include "CollisionManager.h"
#include "Enemy.h"
#include "GameDefine.h"
#include "CollisionConfig.h"
#include "TimeManager.h"
#include "AudioPlayer.h"
#include "TreeField.h"
#include "Easing.h"
#include "MathUtils.h"

using namespace FS;

namespace
{
    constexpr float kDegToRad = Math::PI / 180.0f;
    constexpr float kEjectShrinkTime = 0.2f;    // 消える前に小さくなる時間

    // マガジンを出したときの音。Assets/Audio に置けば鳴る
    const char* const kMagazineEjectSound = "magazineEject";
}

PlayerWeapon::PlayerWeapon(FS::Engine* engine)
    : engine_(engine), random_(std::random_device{}())
{
    model_ = std::make_unique<FS::Model>(engine_, "P365");
    magazineModel_ = std::make_unique<FS::Model>(engine_, "cube");
    casingModel_ = std::make_unique<FS::Model>(engine_, "bullet");

    binder_ = std::make_unique<FS::PropertyBinder>(engine_, "P365");
}

PlayerWeapon::~PlayerWeapon()
{

}

void PlayerWeapon::Initialize()
{
    model_->MakeMaterialUnique();

    model_->GetTransform().SetParent(&reloadTiltTransform_);
    reloadTiltTransform_.SetParent(&handTransform_);
    magazineModel_->MakeMaterialUnique();
    casingModel_->MakeMaterialUnique();

    ParticleSystem* particleSystem = engine_->GetParticleSystem();
    muzzleFlashEmitterPtr_ = particleSystem->SpawnEmitter("muzzleFlash");
    shotSmokeEmitterPtr_ = particleSystem->SpawnEmitter("shotSmokeParticle");
    shotSparkEmitterPtr_ = particleSystem->SpawnEmitter("shotSparkParticle");
    bulletTracerEmitterPtr_ = particleSystem->SpawnEmitter("bulletTracer");
    woodHitBulletEmitterPtr_ = particleSystem->SpawnEmitter("woodHitBullet");

    // Binderへ登録
    binder_->BindModel("P365Model", model_.get());
    binder_->BindColor("Muzzle Flash Color", &config_.muzzleFlashColor, { 1.0f, 0.75f, 0.3f, 1.0f });
    binder_->Bind("Muzzle Flash Intensity", &config_.muzzleFlashIntensity, 25.0f, 0.5f, 0.0f, 100.0f);
    binder_->Bind("Muzzle Flash Radius", &config_.muzzleFlashRadius, 8.0f, 0.1f, 0.5f, 30.0f);
    binder_->Bind("Muzzle Flash Volumetric", &config_.muzzleFlashVolumetricIntensity, 1.0f, 0.05f, 0.0f, 50.0f);
    binder_->Bind("Muzzle Flash Duration", &config_.muzzleFlashDuration, 0.05f, 0.005f, 0.01f, 0.2f);
    binder_->Bind("Muzzle Offset", &config_.muzzleOffset, { 0.0f, 0.05f, 0.35f });
    binder_->Bind("BaseDamage", &config_.baseDamage, 1);
    binder_->Bind("MaxDistance", &config_.maxDistance, 5.0f);
    binder_->Bind("MagazineSize", &config_.magazineSize, 10, 1.0f, 1, 200);
    binder_->Bind("FireInterval", &config_.fireInterval, 0.35f, 0.01f, 0.0f, 5.0f);
    binder_->Bind("ReloadTime", &config_.reloadTime, 1.6f, 0.05f, 0.05f, 10.0f);

    binder_->BindModel("MagazineModel", magazineModel_.get());
    binder_->BindModel("CasingModel", casingModel_.get());
    binder_->Bind("Reload Tilt Axis", &config_.reloadTiltAxis, { 1.0f, 0.0f, 0.0f });
    binder_->Bind("Reload Tilt Angle", &config_.reloadTiltAngle, 35.0f, 0.5f, -180.0f, 180.0f);
    binder_->Bind("Reload Tilt In Time", &config_.reloadTiltInTime, 0.15f, 0.01f, 0.01f, 1.0f);
    binder_->Bind("Reload Tilt Out Time", &config_.reloadTiltOutTime, 0.15f, 0.01f, 0.01f, 1.0f);
    BindEjectSettings("Magazine", config_.magazine);
    BindEjectSettings("Casing", config_.casing);

    Refill();
}

void PlayerWeapon::StartReload()
{
    if (IsReloading() || ammoInMagazine_ >= config_.magazineSize)
    {
        return;
    }
    reloadTimer_ = config_.reloadTime;
    DropMagazine();
}

void PlayerWeapon::Refill()
{
    ammoInMagazine_ = config_.magazineSize;
    reloadTimer_ = 0.0f;
    fireCooldown_ = 0.0f;
}

void PlayerWeapon::Update(const Matrix4x4& handWorldMatrix, Camera* camera)
{
    const float deltaTime = TimeManager::GetInstance()->GetDeltaTime();
    fireCooldown_ = std::max(fireCooldown_ - deltaTime, 0.0f);

    if (IsReloading())
    {
        reloadTimer_ -= deltaTime;
        if (reloadTimer_ <= 0.0f)
        {
            reloadTimer_ = 0.0f;
            ammoInMagazine_ = config_.magazineSize;
            ++reloadCount_;
        }
    }

    camera_ = camera;

    // 右手のワールド行列を親トランスフォームに設定。武器のモデルは子なので、読んだときに付いてくる
    handTransform_.SetWorldMatrix(handWorldMatrix);

    if (config_.reloadTiltAxis.LengthSq() > 0.0001f)
    {
        const float tiltAngle = config_.reloadTiltAngle * kDegToRad * CalculateReloadTilt();
        reloadTiltTransform_.SetRotation(Quaternion::FromAxisAngle(config_.reloadTiltAxis.Normalize(), tiltAngle));
    }

    for (EjectedObject& ejectedObject : ejectedObjects_)
    {
        UpdateEjectedObject(ejectedObject, deltaTime);
    }
    std::erase_if(ejectedObjects_, [](const EjectedObject& ejectedObject) {
        return ejectedObject.elapsedTime >= ejectedObject.settings->lifeTime;
    });

    // 正しいワールド行列から銃口位置を取得
    Vector3 muzzlePos = GetMuzzleWorldPosition();

    // パーティクルの位置と向きの更新
    if (muzzleFlashEmitterPtr_)
    {
        muzzleFlashEmitterPtr_->SetPosition(muzzlePos);
        if (camera)
        {
            Vector3 forward = camera->GetWorldTransform().rotation_.RotateVector({ 0.0f, 0.0f, 1.0f }).Normalize();
            Quaternion rot = Quaternion::LookRotation(forward, { 0.0f, 1.0f, 0.0f });
            muzzleFlashEmitterPtr_->SetRotation(rot);
        }
    }

    if (shotSmokeEmitterPtr_)
    {
        shotSmokeEmitterPtr_->SetPosition(muzzlePos);
        if (camera)
        {
            Vector3 forward = camera->GetWorldTransform().rotation_.RotateVector({ 0.0f, 0.0f, 1.0f }).Normalize();
            Quaternion rot = Quaternion::LookRotation(forward, { 0.0f, 1.0f, 0.0f });
            shotSmokeEmitterPtr_->SetRotation(rot);
        }
    }

    if (shotSparkEmitterPtr_)
    {
        shotSparkEmitterPtr_->SetPosition(muzzlePos);
        if (camera)
        {
            Vector3 forward = camera->GetWorldTransform().rotation_.RotateVector({ 0.0f, 0.0f, 1.0f }).Normalize();
            Quaternion rot = Quaternion::LookRotation(forward, { 0.0f, 1.0f, 0.0f });
            shotSparkEmitterPtr_->SetRotation(rot);
        }
    }

    // フラッシュライトのタイマー処理
    if (muzzleFlashTimer_ > 0.0f)
    {
        muzzleFlashTimer_ -= TimeManager::GetInstance()->GetDeltaTime();

        float alpha = std::clamp(muzzleFlashTimer_ / config_.muzzleFlashDuration, 0.0f, 1.0f);
        float currentIntensity = config_.muzzleFlashIntensity * alpha;
        float currentVolumetric = config_.muzzleFlashVolumetricIntensity * alpha;

        engine_->GetLightManager()->SubmitPointLight(
            muzzlePos,
            config_.muzzleFlashColor,
            currentIntensity,
            config_.muzzleFlashRadius,
            currentVolumetric
        );
    }
}

Vector3 PlayerWeapon::GetMuzzleWorldPosition() const
{
    if (model_)
    {
        return model_->GetTransform().GetWorldMatrix().TransformPoint(config_.muzzleOffset);
    }
    return currentHandMatrix_.TransformPoint(config_.muzzleOffset);
}

bool PlayerWeapon::Fire(Camera* camera, float focusRatio, float maxDamageMultiplier, float maxBulletSpread, CollisionManager* colManager,
    TreeField* treeField)
{
    if (!camera || !colManager || !CanFire()) return false;

    --ammoInMagazine_;
    fireCooldown_ = config_.fireInterval;

    AudioPlayer::GetInstance().Play("gunShot", 0.2f);

    muzzleFlashTimer_ = config_.muzzleFlashDuration;
    if (muzzleFlashEmitterPtr_) muzzleFlashEmitterPtr_->Play();
    if (shotSmokeEmitterPtr_)   shotSmokeEmitterPtr_->Play();
    if (shotSparkEmitterPtr_)   shotSparkEmitterPtr_->Play();
    Eject(config_.casing, "bullet", casingModel_.get());

    // カメラからのレイとレティクル拡散の計算
    Vector3 rayStart = camera->GetWorldTransform().translation_;
    Vector3 baseForward = camera->GetWorldTransform().rotation_.RotateVector({ 0.0f, 0.0f, 1.0f }).Normalize();

    float currentSpread = maxBulletSpread * (1.0f - focusRatio);
    float randPitch = (((float)rand() / RAND_MAX) * 2.0f - 1.0f) * currentSpread;
    float randYaw = (((float)rand() / RAND_MAX) * 2.0f - 1.0f) * currentSpread;

    Quaternion spreadRot = Quaternion::QuaternionFromEuler({ randPitch, randYaw, 0.0f });
    Vector3 finalRayDir = spreadRot.RotateVector(baseForward).Normalize();

    float damageMult = 1.0f + (maxDamageMultiplier - 1.0f) * focusRatio;
    int finalDamage = static_cast<int>(config_.baseDamage * damageMult);

    // ターゲット位置の決定
    Vector3 targetPoint = rayStart + finalRayDir * config_.maxDistance;

    RaycastHit hitInfo;
    uint32_t targetMask = CollisionLayer::Enemy | CollisionLayer::Prop;
    const bool hasColliderHit = colManager->Raycast(rayStart, finalRayDir, config_.maxDistance, &hitInfo, targetMask);

    // レイはカメラから飛ばすので、カメラとプレイヤーの間にある木は無視する
    const float muzzleDistance = (GetMuzzleWorldPosition() - rayStart).Dot(finalRayDir);
    TreeRaycastHit treeHit;
    const bool hasTreeHit = treeField && treeField->Raycast(rayStart, finalRayDir, config_.maxDistance, &treeHit)
        && treeHit.distance > muzzleDistance;

    // 木が手前にあれば弾はそこで止まり、奥の敵には当たらない
    const bool isTreeInFront = hasTreeHit && (!hasColliderHit || treeHit.distance < hitInfo.distance);

    if (isTreeInFront)
    {
        targetPoint = treeHit.point;

        // 木の表面の法線の方向へ木屑が飛び散るようにする
        if (woodHitBulletEmitterPtr_)
        {
            woodHitBulletEmitterPtr_->SetPosition(treeHit.point);
            woodHitBulletEmitterPtr_->SetRotation(Quaternion::LookRotation(treeHit.normal, { 0.0f, 1.0f, 0.0f }));
            woodHitBulletEmitterPtr_->Play();
        }
    }
    else if (hasColliderHit)
    {
        targetPoint = hitInfo.point;

        if (hitInfo.hitObject && hitInfo.hitObject->CompareTag(ObjectTag::Enemy))
        {
            auto enemy = static_cast<Enemy*>(hitInfo.hitObject);
            enemy->TakeDamage(finalDamage, hitInfo.point, hitInfo.normal, hitInfo.hitCollider);
            AudioPlayer::GetInstance().Play("floatingEnemyDamaged", 0.5f);
        }
    }

    // 銃口から着弾点に向けた弾道の生成
    if (bulletTracerEmitterPtr_)
    {
        Vector3 muzzlePos = GetMuzzleWorldPosition();
        Vector3 bulletDir = (targetPoint - muzzlePos);

        if (bulletDir.LengthSq() > 0.001f)
        {
            bulletDir = bulletDir.Normalize();

            // 銃口からターゲット方向を向く Quaternion を計算
            Quaternion tracerRot = Quaternion::LookRotation(bulletDir, { 0.0f, 1.0f, 0.0f });

            // エミッターの位置と向きをセット
            bulletTracerEmitterPtr_->SetPosition(muzzlePos);
            bulletTracerEmitterPtr_->SetRotation(tracerRot);

            // 発射
            bulletTracerEmitterPtr_->Play();
        }
    }

    return true;
}

void PlayerWeapon::Draw()
{
    if (model_)
    {
        model_->Draw();
    }

    for (const EjectedObject& ejectedObject : ejectedObjects_)
    {
        ejectedObject.model->Draw();
    }
}

// 0 = 構えたまま, 1 = いちばん傾けたところ。始めにすっと傾け、終わりに戻す
float PlayerWeapon::CalculateReloadTilt() const
{
    if (!IsReloading())
    {
        return 0.0f;
    }

    const float elapsedTime = config_.reloadTime - reloadTimer_;
    const float tiltIn = elapsedTime / config_.reloadTiltInTime;
    const float tiltOut = reloadTimer_ / config_.reloadTiltOutTime;
    return Easing::Evaluate(EasingType::EaseOutCubic, std::clamp(std::min(tiltIn, tiltOut), 0.0f, 1.0f));
}

void PlayerWeapon::DropMagazine()
{
    Eject(config_.magazine, "cube", magazineModel_.get());
    AudioPlayer::GetInstance().Play(kMagazineEjectSound, 0.4f);
}

void PlayerWeapon::Eject(const EjectSettings& settings, const std::string& modelName, const Model* materialSource)
{
    const Matrix4x4& gunMatrix = model_->GetTransform().GetWorldMatrix();
    Vector3 gunScale;
    Quaternion gunRotation;
    Vector3 gunTranslation;
    Matrix4x4::Decompose(gunMatrix, gunScale, gunRotation, gunTranslation);

    EjectedObject ejectedObject;
    ejectedObject.settings = &settings;
    ejectedObject.model = std::make_unique<Model>(engine_, modelName);
    if (materialSource)
    {
        ejectedObject.model->ShareMaterialsFrom(materialSource);
    }

    WorldTransform& transform = ejectedObject.model->GetTransform();
    transform.translation_ = gunMatrix.TransformPoint(settings.offset);
    transform.SetRotation(gunRotation);
    transform.scale_ = settings.scale;

    // 向きを少しばらけさせて飛ばし、重力で落とす
    std::uniform_real_distribution<float> spread(-settings.spread, settings.spread);
    // 向きはカメラを基準にする。銃の向きは構え方やリロードの傾きで変わるので、画面の右上へ飛ばすにはカメラのほうが安定する
    const Quaternion& directionBasis = camera_ ? camera_->GetWorldTransform().rotation_ : gunRotation;
    const Vector3 baseDirection = directionBasis.RotateVector(settings.direction).Normalize();
    const Vector3 direction = (baseDirection + Vector3(spread(random_), spread(random_), spread(random_))).Normalize();
    ejectedObject.velocity = direction * settings.speed;

    std::uniform_real_distribution<float> axis(-1.0f, 1.0f);
    ejectedObject.spinAxis = Vector3(axis(random_), axis(random_), axis(random_)).Normalize();
    ejectedObject.spinSpeed = settings.spinSpeed * std::uniform_real_distribution<float>(0.6f, 1.4f)(random_);

    ejectedObjects_.push_back(std::move(ejectedObject));
}

void PlayerWeapon::UpdateEjectedObject(EjectedObject& ejectedObject, float deltaTime)
{
    const EjectSettings& settings = *ejectedObject.settings;
    ejectedObject.elapsedTime += deltaTime;
    WorldTransform& transform = ejectedObject.model->GetTransform();

    ejectedObject.velocity.y -= settings.gravity * deltaTime;
    transform.translation_ += ejectedObject.velocity * deltaTime;
    transform.SetRotation(Quaternion::FromAxisAngle(ejectedObject.spinAxis, ejectedObject.spinSpeed * deltaTime) * transform.rotation_);

    // 消える直前に小さくしてから消す
    const float remainingTime = settings.lifeTime - ejectedObject.elapsedTime;
    transform.scale_ = settings.scale * std::clamp(remainingTime / kEjectShrinkTime, 0.0f, 1.0f);
}

// 初期値は Config に書いた値を使う
void PlayerWeapon::BindEjectSettings(const std::string& prefix, EjectSettings& settings)
{
    binder_->Bind(prefix + " Offset", &settings.offset, settings.offset);
    binder_->Bind(prefix + " Direction", &settings.direction, settings.direction);
    binder_->Bind(prefix + " Scale", &settings.scale, settings.scale, 0.005f);
    binder_->Bind(prefix + " Eject Speed", &settings.speed, settings.speed, 0.1f, 0.0f, 50.0f);
    binder_->Bind(prefix + " Spread", &settings.spread, settings.spread, 0.01f, 0.0f, 2.0f);
    binder_->Bind(prefix + " Spin Speed", &settings.spinSpeed, settings.spinSpeed, 0.1f, 0.0f, 100.0f);
    binder_->Bind(prefix + " Gravity", &settings.gravity, settings.gravity, 0.5f, 0.0f, 200.0f);
    binder_->Bind(prefix + " Life Time", &settings.lifeTime, settings.lifeTime, 0.05f, 0.1f, 10.0f);
}

void PlayerWeapon::DrawEjectSettings(const std::string& prefix)
{
    binder_->Draw(prefix + " Offset", "出る位置 (銃から見て)");
    binder_->Draw(prefix + " Direction", "飛ぶ向き (カメラから見て。x 右, y 上, z 奥)");
    binder_->Draw(prefix + " Scale", "大きさ");
    binder_->Draw(prefix + " Eject Speed", "飛び出す速さ");
    binder_->Draw(prefix + " Spread", "向きのばらつき");
    binder_->Draw(prefix + " Spin Speed", "回る速さ");
    binder_->Draw(prefix + " Gravity", "重力");
    binder_->Draw(prefix + " Life Time", "消えるまで (秒)");
}

void PlayerWeapon::DebugGui()
{
    if (ImGui::CollapsingHeader("武器設定"))
    {
        ImGui::Indent();

        binder_->DrawModel("P365Model", "モデル・オフセット調整");

        ImGui::Spacing();
        ImGui::TextDisabled("マズルフラッシュ設定");
        binder_->Draw("Muzzle Flash Color", "発光色");
        binder_->Draw("Muzzle Flash Intensity", "発光強度");
        binder_->Draw("Muzzle Flash Radius", "照射半径");
        binder_->Draw("Muzzle Flash Volumetric", "ボリュメトリック散乱強度");
        binder_->Draw("Muzzle Flash Duration", "発光時間");
        binder_->Draw("Muzzle Offset", "銃口位置オフセット");

        ImGui::Spacing();
        ImGui::TextDisabled("性能設定");
        binder_->Draw("BaseDamage", "基本ダメージ");
        binder_->Draw("MaxDistance", "最大射程");
        binder_->Draw("MagazineSize", "装弾数");
        binder_->Draw("FireInterval", "1 発ごとの間隔 (秒)");
        binder_->Draw("ReloadTime", "リロード時間 (秒)");
        ImGui::Text("残り: %d / %d %s", ammoInMagazine_, config_.magazineSize, IsReloading() ? "(リロード中)" : "");

        ImGui::Spacing();
        ImGui::TextDisabled("リロードの演出");
        binder_->Draw("Reload Tilt Axis", "銃を傾ける軸 (手から見て)");
        binder_->Draw("Reload Tilt Angle", "傾ける角度 (度)");
        binder_->Draw("Reload Tilt In Time", "傾けるまで (秒)");
        binder_->Draw("Reload Tilt Out Time", "戻すまで (秒)");

        if (ImGui::TreeNode("マガジン"))
        {
            DrawEjectSettings("Magazine");
            binder_->DrawModel("MagazineModel", "マガジンの見た目 (色や質感だけを使う)");
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("薬莢"))
        {
            DrawEjectSettings("Casing");
            binder_->DrawModel("CasingModel", "薬莢の見た目 (色や質感だけを使う)");
            ImGui::TreePop();
        }

        ImGui::Unindent(); 
    }
}