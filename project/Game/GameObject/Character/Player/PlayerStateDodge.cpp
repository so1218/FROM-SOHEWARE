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
    Vector3 dodgeDirection = player->GetMoveDirection();
    if (dodgeDirection.LengthSq() < 0.01f)
    {
        dodgeDirection = player->GetLastMoveDirection();
    }
    dodgeDirection.y = 0.0f;
    dodgeDirection = dodgeDirection.Normalize();

    player->SetDodgeDirection(dodgeDirection);
    player->SetDodgeElapsedTime(0.0f);

    // 回避の向きへすぐ体を向ける
    const float angleY = std::atan2(dodgeDirection.x, dodgeDirection.z);
    player->GetTransform().rotationQuaternion_ = Quaternion::QuaternionFromEuler({ 0.0f, angleY, 0.0f });

    // 回避用のアニメーションを追加したら、ここの名前を変える
    player->PlayAnimation("humanRun", true, player->config.dodgeAnimSpeed, 0.05f);
}

void PlayerStateDodge::Update(Player* player)
{
    const float deltaTime = TimeManager::GetInstance()->GetDeltaTime();
    const float duration = std::max(player->config.dodgeDuration, 0.01f);

    const float elapsedTime = player->GetDodgeElapsedTime() + deltaTime;
    player->SetDodgeElapsedTime(elapsedTime);
    const float progress = std::min(elapsedTime / duration, 1.0f);

    // 出だしが一番速く、最後に止まる。(1 - progress) * 2 を 0〜1 で積分すると 1 なので、ちょうど dodgeDistance 進む
    const float averageSpeed = player->config.dodgeDistance / duration;
    const float currentSpeed = averageSpeed * 2.0f * (1.0f - progress);
    player->ApplyHorizontalMovement(player->GetDodgeDirection(), currentSpeed);

    player->ApplyGravity();
    player->SnapToGround();

    if (progress < 1.0f)
    {
        return;
    }

    if (!player->IsGrounded())
    {
        player->GetStateMachine()->ChangeState(PlayerStateJump::GetInstance());
    }
    else if (player->IsSprintInputActive() && !player->GetStamina()->IsExhausted())
    {
        player->GetStateMachine()->ChangeState(PlayerStateSprint::GetInstance());
    }
    else
    {
        player->GetStateMachine()->ChangeState(PlayerStateNormal::GetInstance());
    }
}
