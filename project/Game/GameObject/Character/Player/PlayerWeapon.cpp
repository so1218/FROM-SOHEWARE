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

using namespace FE;

PlayerWeapon::PlayerWeapon(FE::Engine* engine)
    : engine_(engine)
{
    model_ = std::make_unique<FE::Model>(engine_, "P365");

    binder_ = std::make_unique<FE::PropertyBinder>(engine_, "P365");
}

PlayerWeapon::~PlayerWeapon()
{

}

void PlayerWeapon::Initialize()
{
    model_->MakeMaterialUnique();

    // 武器モデルの親に handTransform_ を設定
    model_->GetTransform().SetParent(&handTransform_);

    auto muzzleParticle = engine_->GetParticleSystem()->CreateEmitter("muzzleFlash");
    if (muzzleParticle)
    {
        muzzleFlashEmitterPtr_ = muzzleParticle.get();
        engine_->GetParticleSystem()->AddEmitter(std::move(muzzleParticle));
    }

    auto shotSmokeParticle = engine_->GetParticleSystem()->CreateEmitter("shotSmokeParticle");
    if (shotSmokeParticle)
    {
        shotSmokeEmitterPtr_ = shotSmokeParticle.get();
        engine_->GetParticleSystem()->AddEmitter(std::move(shotSmokeParticle));
    }

    auto shotSparkParticle = engine_->GetParticleSystem()->CreateEmitter("shotSparkParticle");
    if (shotSparkParticle)
    {
        shotSparkEmitterPtr_ = shotSparkParticle.get();
        engine_->GetParticleSystem()->AddEmitter(std::move(shotSparkParticle));
    }

    auto tracerParticle = engine_->GetParticleSystem()->CreateEmitter("bulletTracer");
    if (tracerParticle)
    {
        bulletTracerEmitterPtr_ = tracerParticle.get();
        engine_->GetParticleSystem()->AddEmitter(std::move(tracerParticle));
    }

    auto woodHitBulletParticle = engine_->GetParticleSystem()->CreateEmitter("woodHitBullet");
    if (woodHitBulletParticle)
    {
        woodHitBulletEmitterPtr_ = woodHitBulletParticle.get();
        engine_->GetParticleSystem()->AddEmitter(std::move(woodHitBulletParticle));
    }

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
}

void PlayerWeapon::Update(const Matrix4x4& handWorldMatrix, Camera* camera)
{
    // 右手のワールド行列を親トランスフォームに設定
    handTransform_.matWorld_ = handWorldMatrix;

    // 武器モデルの更新
    if (model_)
    {
        model_->GetTransform().UpdateMatrix();
    }

    // 正しいワールド行列から銃口位置を取得
    Vector3 muzzlePos = GetMuzzleWorldPosition();

    // パーティクルの位置と向きの更新
    if (muzzleFlashEmitterPtr_)
    {
        muzzleFlashEmitterPtr_->SetPosition(muzzlePos);
        if (camera)
        {
            Vector3 forward = camera->GetWorldTransform().rotationQuaternion_.RotateVector({ 0.0f, 0.0f, 1.0f }).Normalize();
            Quaternion rot = Quaternion::LookRotation(forward, { 0.0f, 1.0f, 0.0f });
            muzzleFlashEmitterPtr_->SetRotation(rot);
        }
    }

    if (shotSmokeEmitterPtr_)
    {
        shotSmokeEmitterPtr_->SetPosition(muzzlePos);
        if (camera)
        {
            Vector3 forward = camera->GetWorldTransform().rotationQuaternion_.RotateVector({ 0.0f, 0.0f, 1.0f }).Normalize();
            Quaternion rot = Quaternion::LookRotation(forward, { 0.0f, 1.0f, 0.0f });
            shotSmokeEmitterPtr_->SetRotation(rot);
        }
    }

    if (shotSparkEmitterPtr_)
    {
        shotSparkEmitterPtr_->SetPosition(muzzlePos);
        if (camera)
        {
            Vector3 forward = camera->GetWorldTransform().rotationQuaternion_.RotateVector({ 0.0f, 0.0f, 1.0f }).Normalize();
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
        return model_->GetTransform().matWorld_.TransformPoint(config_.muzzleOffset);
    }
    return currentHandMatrix_.TransformPoint(config_.muzzleOffset);
}

bool PlayerWeapon::Fire(Camera* camera, float focusRatio, float maxDamageMultiplier, float maxBulletSpread, CollisionManager* colManager,
    TreeField* treeField)
{
    if (!camera || !colManager) return false;

    AudioPlayer::GetInstance().Play("gunShot", false, 20);

    muzzleFlashTimer_ = config_.muzzleFlashDuration;
    if (muzzleFlashEmitterPtr_) muzzleFlashEmitterPtr_->Play();
    if (shotSmokeEmitterPtr_)   shotSmokeEmitterPtr_->Play();
    if (shotSparkEmitterPtr_)   shotSparkEmitterPtr_->Play();

    // カメラからのレイとレティクル拡散の計算
    Vector3 rayStart = camera->GetWorldTransform().translation_;
    Vector3 baseForward = camera->GetWorldTransform().rotationQuaternion_.RotateVector({ 0.0f, 0.0f, 1.0f }).Normalize();

    float currentSpread = maxBulletSpread * (1.0f - focusRatio);
    float randPitch = (((float)rand() / RAND_MAX) * 2.0f - 1.0f) * currentSpread;
    float randYaw = (((float)rand() / RAND_MAX) * 2.0f - 1.0f) * currentSpread;

    Quaternion spreadRot = Quaternion::QuaternionFromEuler({ randPitch, randYaw, 0.0f });
    Vector3 finalRayDir = spreadRot.RotateVector(baseForward).Normalize();

    float damageMult = 1.0f + (maxDamageMultiplier - 1.0f) * focusRatio;
    int finalDamage = static_cast<int>(config_.baseDamage * damageMult);

    // ターゲット位置の決定
    // デフォルトは最大射程先の座標
    Vector3 targetPoint = rayStart + finalRayDir * config_.maxDistance;

    RaycastHit hitInfo;
    uint32_t targetMask = kCollisionAttributeEnemy | kCollisionAttributeProp;

    if (colManager->Raycast(rayStart, finalRayDir, config_.maxDistance, &hitInfo, targetMask))
    {
        // レティクルが当たった実際の地点を着弾点とする
        targetPoint = hitInfo.point;

        if (hitInfo.hitObject && hitInfo.hitObject->CompareTag(ObjectTag::Enemy))
        {
            auto enemy = static_cast<Enemy*>(hitInfo.hitObject);
            if (enemy)
            {
                enemy->TakeDamage(finalDamage, hitInfo.point, hitInfo.normal);
                AudioPlayer::GetInstance().Play("floatingEnemyDamaged", false, 40);
            }
        }
    }

    // 木への Raycast 判定
    TreeRaycastHit treeHit;
    if (treeField && treeField->Raycast(rayStart, finalRayDir, config_.maxDistance, &treeHit))
    {
        // 敵などよりも手前の木に当たった場合
        targetPoint = treeHit.point;

        // 木への着弾エフェクトを発生
        if (woodHitBulletEmitterPtr_)
        {
            woodHitBulletEmitterPtr_->SetPosition(treeHit.point);

            // 木の表面の法線の方向へ木屑が飛び散るように回転をセット
            Quaternion rot = Quaternion::LookRotation(treeHit.normal, { 0.0f, 1.0f, 0.0f });
            woodHitBulletEmitterPtr_->SetRotation(rot);

            woodHitBulletEmitterPtr_->Play(); 
        }
    }

    // 銃口から着弾点に向けた弾道の生成
    if (bulletTracerEmitterPtr_)
    {
        Vector3 muzzlePos = GetMuzzleWorldPosition(); // 銃口のワールド座標
        Vector3 bulletDir = (targetPoint - muzzlePos);  // 銃口からターゲットへのベクトル

        if (bulletDir.LengthSq() > 0.001f)
        {
            bulletDir = bulletDir.Normalize();

            // 銃口からターゲット方向を向く Quaternion を計算
            Quaternion tracerRot = Quaternion::LookRotation(bulletDir, { 0.0f, 1.0f, 0.0f });

            // エミッターの位置と向きをセット
            bulletTracerEmitterPtr_->SetPosition(muzzlePos);
            bulletTracerEmitterPtr_->SetRotation(tracerRot);

            // 1発発射
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
}

void PlayerWeapon::DebugDraw()
{
#ifdef ENABLE_IMGUI
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

        ImGui::Unindent(); 
    }
#endif
}