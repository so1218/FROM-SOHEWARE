#include "pch.h"
#include "PlayerWeapon.h"
#include "Engine.h"
#include "CollisionManager.h"
#include "Enemy.h"
#include "GameDefine.h"
#include "CollisionConfig.h"
#include "TimeManager.h"
#include "AudioPlayer.h"

using namespace FE;

PlayerWeapon::PlayerWeapon(FE::Engine* engine)
    : engine_(engine)
{
    model_ = std::make_unique<FE::Model>(engine_, "P365");

    binder_ = std::make_unique<FE::PropertyBinder>(engine_, "P365");
}

PlayerWeapon::~PlayerWeapon()
{
    if (muzzleLightIndex_ >= 0 && engine_)
    {
        engine_->GetLightManager()->ReturnPointLight(muzzleLightIndex_);
    }
}

void PlayerWeapon::Initialize()
{
    model_->MakeMaterialUnique();

    // 武器モデルの親に handTransform_ を設定
    model_->GetTransform().SetParent(&handTransform_);

    muzzleLightIndex_ = engine_->GetLightManager()->RequestPointLight();
    if (muzzleLightIndex_ >= 0)
    {
        engine_->GetLightManager()->UpdatePointLightProperties(
            muzzleLightIndex_, config_.muzzleFlashColor, 0.0f, config_.muzzleFlashRadius, 0.0f
        );
    }

    auto muzzleParticle = engine_->GetParticleSystem()->CreateEmitter("muzzleFlash");
    if (muzzleParticle)
    {
        muzzleFlashEmitterPtr_ = muzzleParticle.get();
        engine_->GetParticleSystem()->AddEmitter(std::move(muzzleParticle));
    }

    // 武器専用Binderへ登録
    binder_->BindModel("P365Model", model_.get());
    binder_->Bind("Muzzle Flash Color", &config_.muzzleFlashColor, { 1.0f, 0.75f, 0.3f, 1.0f });
    binder_->Bind("Muzzle Flash Intensity", &config_.muzzleFlashIntensity, 25.0f, 0.5f, 0.0f, 100.0f);
    binder_->Bind("Muzzle Flash Radius", &config_.muzzleFlashRadius, 8.0f, 0.1f, 0.5f, 30.0f);
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

    // フラッシュライトのタイマー処理
    if (muzzleLightIndex_ >= 0)
    {
        if (muzzleFlashTimer_ > 0.0f)
        {
            muzzleFlashTimer_ -= TimeManager::GetInstance()->GetDeltaTime();
            engine_->GetLightManager()->UpdatePointLightPosition(muzzleLightIndex_, muzzlePos);

            float alpha = std::clamp(muzzleFlashTimer_ / config_.muzzleFlashDuration, 0.0f, 1.0f);
            float currentIntensity = config_.muzzleFlashIntensity * alpha;

            engine_->GetLightManager()->UpdatePointLightProperties(
                muzzleLightIndex_, config_.muzzleFlashColor, currentIntensity, config_.muzzleFlashRadius, 1.0f
            );
        }
        else
        {
            engine_->GetLightManager()->UpdatePointLightPosition(muzzleLightIndex_, muzzlePos);
            engine_->GetLightManager()->UpdatePointLightProperties(
                muzzleLightIndex_, config_.muzzleFlashColor, 0.0f, config_.muzzleFlashRadius, 0.0f
            );
        }
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

bool PlayerWeapon::Fire(Camera* camera, float focusRatio, float maxDamageMultiplier, float maxBulletSpread, CollisionManager* colManager)
{
    if (!camera || !colManager) return false;

    AudioPlayer::GetInstance().Play("gunShot", false, 20);

    muzzleFlashTimer_ = config_.muzzleFlashDuration;
    if (muzzleFlashEmitterPtr_)
    {
        muzzleFlashEmitterPtr_->Play();
    }

    Vector3 rayStart = camera->GetWorldTransform().translation_;
    Vector3 baseForward = camera->GetWorldTransform().rotationQuaternion_.RotateVector({ 0.0f, 0.0f, 1.0f }).Normalize();

    // 拡散（スプレッド）計算
    float currentSpread = maxBulletSpread * (1.0f - focusRatio);
    float randPitch = (((float)rand() / RAND_MAX) * 2.0f - 1.0f) * currentSpread;
    float randYaw = (((float)rand() / RAND_MAX) * 2.0f - 1.0f) * currentSpread;

    Quaternion spreadRot = Quaternion::QuaternionFromEuler({ randPitch, randYaw, 0.0f });
    Vector3 finalRayDir = spreadRot.RotateVector(baseForward).Normalize();

    float damageMult = 1.0f + (maxDamageMultiplier - 1.0f) * focusRatio;
    int finalDamage = static_cast<int>(config_.baseDamage * damageMult);

    RaycastHit hitInfo;
    uint32_t targetMask = kCollisionAttributeEnemy | kCollisionAttributeProp;

    if (colManager->Raycast(rayStart, finalRayDir, config_.maxDistance, &hitInfo, targetMask))
    {
        if (hitInfo.hitObject && hitInfo.hitObject->CompareTag(ObjectTag::Enemy))
        {
            auto* enemy = static_cast<Enemy*>(hitInfo.hitObject);
            if (enemy)
            {
                enemy->TakeDamage(finalDamage, hitInfo.point, hitInfo.normal);
                AudioPlayer::GetInstance().Play("floatingEnemyDamaged", false, 40);
            }
        }
    }

    return true; // 射撃成功
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
    binder_->DrawModel("P365Model", "P365 武器モデル・オフセット");

    ImGui::Separator();
    ImGui::Text("マズルフラッシュ設定");
    binder_->Draw("Muzzle Flash Color", "発光色");
    binder_->Draw("Muzzle Flash Intensity", "発光強度");
    binder_->Draw("Muzzle Flash Radius", "照射半径");
    binder_->Draw("Muzzle Flash Duration", "発光時間");
    binder_->Draw("Muzzle Offset", "銃口位置オフセット");

    ImGui::Separator();
    ImGui::Text("性能設定");
    binder_->Draw("BaseDamage", "基本ダメージ");
    binder_->Draw("MaxDistance", "最大射程");
#endif
}