#include "pch.h"
#include "Enemy.h"
#include "GameDefine.h"
#include "TimeManager.h"
#include "CollisionConfig.h"
#include "FloatingBehavior.h"

using namespace FE;

Enemy::Enemy(FE::Engine* engine, int id, EnemyType type, const std::string& parentGroupName)
    : engine_(engine), id_(id), type_(type)
{
    std::string childGroupName = "Enemy_" + std::to_string(id_);
    binder_ = std::make_unique<FE::PropertyBinder>(engine_, parentGroupName, childGroupName);

    // ★ ここがファクトリー（工場）の役割。Typeによって取り付けるパーツとモデルを変える
    std::string modelName = "enemy";

    switch (type_)
    {
    case EnemyType::Floating:
        modelName = "enemy"; // 浮遊用モデル
        behavior_ = std::make_unique<FloatingBehavior>();
        break;
        // 将来はここに case EnemyType::Zombie: などを足すだけ
    }

    model_ = std::make_unique<FE::Model>(engine_, modelName);
    collider_ = std::make_unique<FE::Collider>(this);
}

void Enemy::Initialize()
{
    collider_->RegisterToManager();
    SetTag(ObjectTag::Enemy);

    // 共通プロパティのバインド
    binder_->Bind("Scale", &scale_, { 1.0f, 1.0f, 1.0f });
    binder_->Bind("ColliderRadius", &colliderRadius_, 1.0f);
    binder_->Bind("ColliderOffset", &colliderOffset_, { 0.0f, 0.0f, 0.0f });

    collider_->SetApplyRotation(false);
    collider_->SetCollisionAttribute(kCollisionAttributeEnemy);
    collider_->SetCollisionMask(kCollisionAttributePlayer);

    // パーティクル等の共通セットアップ
    auraEmitter_ = engine_->GetParticleSystem()->CreateEmitter("enemyAura");
    auraEmitter_->SetTargetToFollow(&model_->GetTransform());
    engine_->GetParticleSystem()->AddEmitter(std::move(auraEmitter_));

    // ★ 固有の振る舞いを初期化
    if (behavior_) behavior_->Initialize(this);
}

void Enemy::Update()
{
    if (IsDead()) return;

    // 固有の振る舞いを更新
    if (behavior_) behavior_->Update(this);

    model_->GetTransform().scale_ = scale_;
    collider_->SetRadius(colliderRadius_);
    collider_->SetCenterOffset(colliderOffset_);
}

void Enemy::Draw()
{
    if (model_) model_->Draw();
    collider_->DrawCollider();
}

void Enemy::DebugDraw()
{
#ifdef ENABLE_IMGUI
    ImGui::PushID(id_);
    std::string headerName = "敵" + std::to_string(id_) + " の設定";

    if (ImGui::CollapsingHeader(headerName.c_str()))
    {
        ImGui::Text("基本設定");
        binder_->Draw("Scale", "スケール");

        ImGui::Text("当たり判定設定");
        binder_->Draw("ColliderRadius", "半径");
        binder_->Draw("ColliderOffset", "オフセット");

        ImGui::Separator();

        // 浮遊敵固有の設定を表示
        if (behavior_)
        {
            behavior_->DebugDraw(this);
        }
    }
    ImGui::PopID();
#endif
}

void Enemy::OnCollisionEnter(FE::Collider* mine, FE::Collider* other)
{
    FE::GameObject* hitObject = other->GetOwner();
    if (hitObject && hitObject->CompareTag(ObjectTag::Player))
    {
        
    }
}