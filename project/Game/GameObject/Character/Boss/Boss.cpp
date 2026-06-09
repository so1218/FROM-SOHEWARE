#include "pch.h"
#include "Boss.h"
#include "GameDefine.h"
#include "BossStateIdle.h"
#include "Player.h"
#include "TimeManager.h"

using namespace FE;

Boss::Boss(FE::Engine* engine, Player* player)
    : GameObject(), engine_(engine), player_(player)
{
    // タグの設定
    SetTag(ObjectTag::Enemy);

    // アニメーションモデルの生成
    animation_ = std::make_unique<FE::AnimationModel>(engine_, "playerMesh", "playerWalk");

    // ImGuiバインダーの生成
    binder_ = std::make_unique<FE::PropertyBinder>(engine, "Boss");
}

// 初期化
void Boss::Initialize()
{
    // アニメーションをバインド
    binder_->BindAnimationModel("BossModel", animation_.get());

    // パラメータをバインド
    binder_->Bind("WalkSpeed", &walkSpeed_, 0.01f);
    binder_->Bind("RunSpeed", &runSpeed_, 0.01f);
    binder_->Bind("JumpAttackSpeed", &jumpAttackSpeed_, 0.01f);
    binder_->Bind("RotateSpeed", &rotateSpeed_, 0.1f);
    binder_->Bind("MeleeRange", &meleeRange_, 0.1f);
    binder_->Bind("JumpAttackRange", &jumpAttackRange_, 0.1f);

    // ステートマシンの初期化と最初のステート設定
    stateMachine_ = std::make_unique<StateMachine<Boss>>(this);
    stateMachine_->ChangeState(BossStateIdle::GetInstance());
}

// 更新処理
void Boss::Update()
{
    // 現在のステートを更新（IdleやApproachなどが呼ばれる）
    stateMachine_->Update();

    // 最終的な行列更新
    animation_->Update();
    animation_->GetTransform() = GetTransform();
    GetTransform().UpdateMatrix();
}

// 描画処理
void Boss::Draw()
{
    animation_->Draw();
}

// デバッグ描画
void Boss::DebugDraw()
{
#ifdef IS_DEVELOPMENT
    ImGui::Begin("ボス");

    binder_->DrawAnimationModel("BossModel", "ボスインスペクター");

    ImGui::Separator();

    if (ImGui::CollapsingHeader("動きパラメータ", ImGuiTreeNodeFlags_DefaultOpen))
    {
        binder_->Draw("WalkSpeed", "歩き速度");
        binder_->Draw("RunSpeed", "走り速度");
        binder_->Draw("JumpAttackSpeed", "飛び込み速度");
        binder_->Draw("RotateSpeed", "旋回速度");
    }

    if (ImGui::CollapsingHeader("AI 索敵・攻撃範囲", ImGuiTreeNodeFlags_DefaultOpen))
    {
        binder_->Draw("MeleeRange", "近接攻撃の射程");
        binder_->Draw("JumpAttackRange", "飛び込み攻撃の射程");
    }

    ImGui::Separator();

    // 現在のステート名を表示（デバッグで非常に便利です）
    ImGui::Text("現在のステート: %s", stateMachine_->GetCurrentStateName().c_str());
    ImGui::Text("プレイヤーとの距離: %.2f", GetDistanceToPlayer());

    ImGui::End();
#endif
}

// プレイヤーへの方向ベクトルを取得
FE::Vector3 Boss::GetDirectionToPlayer()
{
    if (!player_) return { 0.0f, 0.0f, 1.0f };

    FE::Vector3 dir = player_->GetTransform().translation_ - GetTransform().translation_;
    dir.y = 0.0f; 

    if (dir.Length() > 0.001f) {
        return dir.Normalize();
    }
    return GetTransform().rotationQuaternion_.RotateVector({ 0.0f, 0.0f, 1.0f });
}

// プレイヤーとの距離を取得
float Boss::GetDistanceToPlayer()
{
    if (!player_) return 9999.0f; 

    FE::Vector3 diff = player_->GetTransform().translation_ - GetTransform().translation_;
    diff.y = 0.0f; // 水平距離で測る
    return diff.Length();
}

// 旋回処理
void Boss::RotateTowards(const FE::Vector3& direction, float speed)
{
    if (direction.Length() < 0.001f) return;

    float deltaTime = TimeManager::GetInstance()->GetDeltaTime();

    // 目標の角度（Y軸回転）を算出
    float targetAngleY = std::atan2(direction.x, direction.z);
    FE::Quaternion targetRotation = FE::Quaternion::QuaternionFromEuler({ 0.0f, targetAngleY, 0.0f });

    // 現在の回転から目標の回転へ球面線形補間（Slerp）
    FE::Quaternion currentRotation = GetTransform().rotationQuaternion_;
    float slerpFactor = FE::Math::Clamp(speed * deltaTime, 0.0f, 1.0f);

    GetTransform().rotationQuaternion_ = FE::Quaternion::Slerp(currentRotation, targetRotation, slerpFactor);
}