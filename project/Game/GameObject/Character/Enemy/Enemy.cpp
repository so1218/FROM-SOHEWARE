#include "pch.h"
#include "Enemy.h"
#include "GameDefine.h"
#include "TimeManager.h"
#include "CollisionConfig.h"

using namespace FS;

Enemy::Enemy(Engine* engine, int id, EnemyType type, std::unique_ptr<IEnemyBehavior> behavior, const std::string& parentGroupName)
    : engine_(engine), id_(id), type_(type), behavior_(std::move(behavior))
{
    std::string childGroupName = "Enemy_" + std::to_string(id_);
    binder_ = std::make_unique<FS::PropertyBinder>(engine_, parentGroupName, childGroupName);

    std::string modelName;
    switch (type_)
    {
    case EnemyType::Floating:
        modelName = "Enemy";
        break;
    }

    model_ = std::make_unique<FS::Model>(engine_, modelName);
    collider_ = AddBoxCollider(behavior_->GetBodyConfig().colliderSize);
    collider_->SetRotateCenterOffset(false);
    collider_->SetCollisionLayer(CollisionLayer::Enemy);
    collider_->SetCollisionMask(CollisionLayer::Player);
}

void Enemy::Initialize()
{
    SetTag(ObjectTag::Enemy);

    // Behavior の初期化
    if (behavior_) behavior_->Initialize(this);

    // Behavior から敵固有の初期HPと被弾パーティクルを取得
    if (behavior_)
    {
        hp_ = behavior_->GetBodyConfig().maxHP;
        std::string particleName = behavior_->GetDamageParticleName();

        damageParticlePtr_ = engine_->GetParticleSystem()->SpawnEmitter(particleName);
    }

}

void Enemy::Update()
{
    if (IsDead() || !IsActive()) return;

    if (behavior_) behavior_->Update(this);

    // 同じ種類の敵で共有している値なので、調整中の変更がすぐ反映されるよう毎フレーム読む
    const EnemyBodyConfig& body = behavior_->GetBodyConfig();
    model_->GetTransform().scale_ = body.scale;
    collider_->SetSize(body.colliderSize);
    collider_->SetCenterOffset(body.colliderOffset);
}

void Enemy::TakeDamage(int damage, const Vector3& hitPoint, const Vector3& hitNormal, const FS::Collider* hitCollider)
{
    if (!IsActive()) return;

    if (behavior_)
    {
        damage = behavior_->ModifyDamage(this, damage, hitCollider);
    }
    hp_ -= damage;

    // 被弾パーティクルの再生
    if (damageParticlePtr_)
    {
        // 着弾座標をセット
        damageParticlePtr_->SetPosition(hitPoint);

        // 着弾面の法線ベクトルから回転を作成してセット
        Quaternion rot = Quaternion::LookRotation(hitNormal, { 0.0f, 1.0f, 0.0f });
        damageParticlePtr_->SetRotation(rot);

        // 再生
        damageParticlePtr_->Play();
    }

    // 敵固有の被弾リアクション
    if (behavior_)
    {
        behavior_->OnTakeDamage(this, damage, hitPoint, hitNormal);
    }

    // 死亡判定。墜落などの演出がある敵は、演出のあとで自分から Die() する
    if (hp_ <= 0 && !(behavior_ && behavior_->OnHealthDepleted(this)))
    {
        Die();
    }
}

void Enemy::Die()
{
    isDefeated_ = true;
    if (behavior_) behavior_->OnDeath(this);
    SetActive(false);
}

void Enemy::Despawn()
{
    if (behavior_) behavior_->OnDespawn(this);
    if (damageParticlePtr_)
    {
        damageParticlePtr_->Destroy();
        damageParticlePtr_ = nullptr;
    }
    SetActive(false);
}

void Enemy::OnCollisionEnter(FS::Collider* mine, FS::Collider* other)
{
    FS::GameObject* hitObject = other ? other->GetOwner() : nullptr;
    if (hitObject && behavior_)
    {
        behavior_->OnCollisionEnter(this, hitObject);
    }
}

void Enemy::Draw()
{
    if (!IsActive()) return;

    if (model_) model_->Draw();
    if (behavior_) behavior_->Draw(this);
}

void Enemy::DebugGui()
{
    ImGui::PushID(id_);
    std::string headerName = "敵 " + std::to_string(id_) + " の設定";

    if (ImGui::CollapsingHeader(headerName.c_str()))
    {
        ImGui::Text("HP: %d", hp_);
        if (behavior_)
        {
            behavior_->DebugGui(this);
        }
    }
    ImGui::PopID();
}
