#include "pch.h"
#include "PlayerStateNormal.h"
#include "PlayerStateJump.h"
#include "PlayerStateAiming.h"
#include "PlayerStateSprint.h"
#include "PlayerStamina.h"

using namespace FS;

void PlayerStateNormal::Update(Player* player)
{
    player->ApplyGravity();
    player->SnapToGround();

    // 足場から外れたら落ちる
    if (!player->IsGrounded())
    {
        player->GetStateMachine()->ChangeState<PlayerStateJump>();
        return;
    }

    const Vector3& moveDirection = player->GetMoveDirection();
    const bool isMoving = player->IsMoving();
    if (isMoving)
    {
        player->PlayAnimation("humanRun", true, player->config.runAnimSpeed, player->config.idleToRunBlendTime);
        player->UpdateRotation(moveDirection);
        player->ApplyHorizontalMovement(moveDirection, player->config.runSpeed);
        player->UpdateFootstepEvents();
    }
    else
    {
        player->PlayAnimation("humanIdle", true, player->config.idleAnimSpeed, player->config.runToIdleBlendTime);
        player->ResetFootstepState();
    }

    if (player->TryStartDodge())
        return;

    if (player->IsAimInputActive())
    {
        player->GetStateMachine()->ChangeState<PlayerStateAiming>();
        return;
    }

    if (isMoving && player->IsSprintInputActive() && !player->GetStamina()->IsExhausted())
    {
        player->GetStateMachine()->ChangeState<PlayerStateSprint>();
        return;
    }

    player->TryStartJump();
}
