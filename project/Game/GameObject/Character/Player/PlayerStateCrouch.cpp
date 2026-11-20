#include "pch.h"
#include "PlayerStateCrouch.h"
#include "PlayerStateNormal.h"
#include "PlayerStateJump.h"
#include "PlayerStateAiming.h"
#include "PlayerStateSprint.h"
#include "PlayerStamina.h"
#include "AnimationManager.h"

using namespace FS;

namespace
{

const char* SelectAnimation(const char* crouchName, const char* fallbackName)
{
    return AnimationManager::GetInstance()->Get(crouchName) ? crouchName : fallbackName;
}

}

void PlayerStateCrouch::Update(Player* player)
{
    player->ApplyGravity();
    player->SnapToGround();

    if (!player->IsGrounded())
    {
        player->GetStateMachine()->ChangeState<PlayerStateJump>();
        return;
    }

    // 足音は鳴らさない
    const Vector3& moveDirection = player->GetMoveDirection();
    if (player->IsMoving())
    {
        player->PlayAnimation(SelectAnimation("humanCrouchWalk", "humanRun"), true, player->config.crouchWalkAnimSpeed, player->config.crouchBlendTime);
        player->UpdateRotation(moveDirection);
        player->ApplyHorizontalMovement(moveDirection, player->config.crouchSpeed);
    }
    else
    {
        player->PlayAnimation(SelectAnimation("humanCrouchIdle", "humanIdle"), true, player->config.crouchIdleAnimSpeed, player->config.crouchBlendTime);
    }
    player->ResetFootstepState();

    if (player->IsCrouchInputTriggered())
    {
        player->GetStateMachine()->ChangeState<PlayerStateNormal>();
        return;
    }

    if (player->TryStartDodge())
        return;

    if (player->IsAimInputActive())
    {
        player->GetStateMachine()->ChangeState<PlayerStateAiming>();
        return;
    }

    if (player->IsMoving() && player->IsSprintInputActive() && !player->GetStamina()->IsExhausted())
    {
        player->GetStateMachine()->ChangeState<PlayerStateSprint>();
        return;
    }

    // 跳ぶボタンは、跳ばずに立ち上がる
    if (player->IsJumpInputTriggered())
    {
        player->GetStateMachine()->ChangeState<PlayerStateNormal>();
    }
}
