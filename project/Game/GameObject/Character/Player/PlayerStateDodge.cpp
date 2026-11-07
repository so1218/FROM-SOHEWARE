#include "pch.h"
#include "PlayerStateDodge.h"
#include "PlayerStateNormal.h"
#include "PlayerStateSprint.h"
#include "PlayerStateJump.h"
#include "PlayerStamina.h"
#include "TimeManager.h"

using namespace FS;

void PlayerStateDodge::Enter(Player* player)
{
    direction_ = player->IsMoving() ? player->GetMoveDirection() : player->GetLastMoveDirection();
    direction_.y = 0.0f;
    direction_ = direction_.Normalize();

    // 回避の向きへ体を向ける
    const float angleY = std::atan2(direction_.x, direction_.z);
    player->GetTransform().rotation_ = Quaternion::QuaternionFromEuler({ 0.0f, angleY, 0.0f });

    // 回避用のアニメーション
    player->PlayAnimation("humanRoll", true, player->config.dodgeAnimSpeed, 0.05f);
}

void PlayerStateDodge::Update(Player* player)
{
    const float duration = std::max(player->config.dodgeDuration, 0.01f);
    elapsedTime_ += TimeManager::GetInstance()->GetDeltaTime();
    const float progress = std::min(elapsedTime_ / duration, 1.0f);

    // 出だしが一番速く、最後に止まる
    const float averageSpeed = player->config.dodgeDistance / duration;
    player->ApplyHorizontalMovement(direction_, averageSpeed * 2.0f * (1.0f - progress));

    player->ApplyGravity();
    player->SnapToGround();

    if (progress < 1.0f)
        return;

    if (!player->IsGrounded())
    {
        player->GetStateMachine()->ChangeState<PlayerStateJump>();
    }
    else if (player->IsSprintInputActive() && !player->GetStamina()->IsExhausted())
    {
        player->GetStateMachine()->ChangeState<PlayerStateSprint>();
    }
    else
    {
        player->GetStateMachine()->ChangeState<PlayerStateNormal>();
    }
}
