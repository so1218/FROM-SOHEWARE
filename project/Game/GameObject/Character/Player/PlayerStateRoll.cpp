#include "pch.h"
#include "PlayerStateRoll.h"
#include "PlayerStateNormal.h"
#include "TimeManager.h"

using namespace FE;

void PlayerStateRoll::Enter(Player* p)
{
    timer_ = 0.0f;
    p->stamina_ -= p->rollStaminaCost_; // ローリングの消費スタミナ
    p->isInvincible_ = true;
    p->animationPlayer_->Play("playerRoll"); // ループさせない

    // ローリング方向の決定（入力があればその方向、なければ向いている方向）
    rollDirection_ = p->GetMoveDirection();
    if (rollDirection_.Length() < 0.1f) {
        rollDirection_ = p->GetLastMoveDirection();
    }
    else {
        rollDirection_ = rollDirection_.Normalize();
    }
}

void PlayerStateRoll::Update(Player* p)
{
    float deltaTime = TimeManager::GetInstance()->GetDeltaTime();
    timer_ += deltaTime;

    // 強制スライド移動
    p->GetTransform().translation_ += rollDirection_ * p->rollSpeed_;

    // 回転もスライド方向に合わせる
    float targetAngleY = std::atan2(rollDirection_.x, rollDirection_.z);
    p->GetTransform().rotationQuaternion_ = Quaternion::QuaternionFromEuler({ 0.0f, targetAngleY, 0.0f });

    // 時間が来たらNormal状態に戻る
    if (timer_ >= p->rollDuration_)
    {
        p->GetStateMachine()->ChangeState(PlayerStateNormal::GetInstance());
    }
}

void PlayerStateRoll::Exit(Player* p)
{
    p->isInvincible_ = false; // 無敵終了
}