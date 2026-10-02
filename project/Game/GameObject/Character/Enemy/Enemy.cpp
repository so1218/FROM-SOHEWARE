#include "pch.h"
#include "Enemy.h"
#include "GameDefine.h"
#include "TimeManager.h"
#include "CollisionConfig.h"
#include "FloatingBehavior.h"

using namespace FE;

Enemy::Enemy(Engine* engine, int id, EnemyType type, const std::string& parentGroupName)
    : engine_(engine), id_(id), type_(type)
{
    std::string childGroupName = "Enemy_" + std::to_string(id_);
    binder_ = std::make_unique<FE::PropertyBinder>(engine_, parentGroupName, childGroupName);

    std::string modelName = "enemy";

    switch (type_)
    {
    case EnemyType::Floating:
        modelName = "enemy";
        behavior_ = std::make_unique<FloatingBehavior>();
        break;
        // 将来: case EnemyType::Zombie: behavior_ = std::make_unique<ZombieBehavior>(); break;
    }

    model_ = std::make_unique<FE::Model>(engine_, modelName);
    collider_ = std::make_unique<FE::Collider>(this);
}

void Enemy::Initialize()
{
    collider_->RegisterToManager();
    SetTag(ObjectTag::Enemy);

    binder_->Bind("Scale", &scale_, { 1.0f, 1.0f, 1.0f });
    binder_->Bind("ColliderRadius", &colliderRadius_, 1.0f);
    binder_->Bind("ColliderOffset", &colliderOffset_, { 0.0f, 0.0f, 0.0f });

    collider_->SetApplyRotation(false);
    collider_->SetCollisionAttribute(kCollisionAttributeEnemy);
    collider_->SetCollisionMask(kCollisionAttributePlayer);

    // 1. Behavior の初期化
    if (behavior_) behavior_->Initialize(this);

    // 2. Behavior から敵固有の初期HPと被弾パーティクルを取得
    if (behavior_)
    {
        hp_ = behavior_->GetInitialHP();
        std::string particleName = behavior_->GetDamageParticleName();

        damageParticle_ = engine_->GetParticleSystem()->CreateEmitter(particleName);
        damageParticlePtr_ = damageParticle_.get();
        if (damageParticle_)
        {
            engine_->GetParticleSystem()->AddEmitter(std::move(damageParticle_));
        }
    }

    binder_->Bind("HP", &hp_, hp_);

    // オーラのセットアップ
    auraEmitter_ = engine_->GetParticleSystem()->CreateEmitter("enemyAura");
    if (auraEmitter_)
    {
        auraEmitter_->SetTargetToFollow(&model_->GetTransform());
        engine_->GetParticleSystem()->AddEmitter(std::move(auraEmitter_));
    }
}

void Enemy::Update()
{
    if (IsDead() || !IsActive()) return;

    if (behavior_) behavior_->Update(this);

    model_->GetTransform().scale_ = scale_;
    collider_->SetRadius(colliderRadius_);
    collider_->SetCenterOffset(colliderOffset_);
}

void Enemy::TakeDamage(int damage, const Vector3& hitPoint, const Vector3& hitNormal)
{
    hp_ -= damage;

    // 被弾パーティクルの再生
    if (damageParticlePtr_)
    {
        // 1. 着弾座標をセット
        damageParticlePtr_->SetPosition(hitPoint);

        // 2. 着弾面の法線ベクトルから回転を作成してセット
        Quaternion rot = Quaternion::LookRotation(hitNormal, { 0.0f, 1.0f, 0.0f });
        damageParticlePtr_->SetRotation(rot);

        // 3. 再生（Play 内でタイマーがリセットされるため連続ヒットも安心）
        damageParticlePtr_->Play();
    }

    // 敵固有の被弾リアクション
    if (behavior_)
    {
        behavior_->OnTakeDamage(this, damage, hitPoint, hitNormal);
    }

    // 死亡判定
    if (hp_ <= 0)
    {
        if (behavior_) behavior_->OnDeath(this);
        SetActive(false);
    }
}

void Enemy::OnCollisionEnter(FE::Collider* mine, FE::Collider* other)
{
    FE::GameObject* hitObject = other ? other->GetOwner() : nullptr;
    if (hitObject && behavior_)
    {
        behavior_->OnCollisionEnter(this, hitObject);
    }
}

void Enemy::Draw()
{
    if (!IsActive()) return;

    if (model_) model_->Draw();
    collider_->DrawCollider();
}

void Enemy::DebugDraw()
{
#ifdef ENABLE_IMGUI
    ImGui::PushID(id_);
    std::string headerName = "敵 " + std::to_string(id_) + " の設定";

    if (ImGui::CollapsingHeader(headerName.c_str()))
    {
        ImGui::Text("基本設定");
        binder_->Draw("HP", "HP");
        binder_->Draw("Scale", "スケール");

        ImGui::Text("当たり判定設定");
        binder_->Draw("ColliderRadius", "半径");
        binder_->Draw("ColliderOffset", "オフセット");

        ImGui::Separator();

        if (behavior_)
        {
            behavior_->DebugDraw(this);
        }
    }
    ImGui::PopID();
#endif
}
