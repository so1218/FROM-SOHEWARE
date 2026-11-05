#include "pch.h"
#include "PlayerStateSprint.h"
#include "PlayerStateNormal.h"
#include "PlayerStateJump.h"
#include "PlayerStateAiming.h"
#include "PlayerStamina.h"

using namespace FS;

void PlayerStateSprint::Enter(Player* player)
{
    player->PlayAnimation("humanRun", true, player->config.sprintAnimSpeed, player->config.idleToRunBlendTime);
}

void PlayerStateSprint::Update(Player* player)
{
    PlayerStamina* stamina = player->GetStamina();

    player->ApplyGravity();
    player->SnapToGround();

    if (!player->IsGrounded())
    {
        player->GetStateMachine()->ChangeState<PlayerStateJump>();
        return;
    }

    if (!player->IsMoving() || !player->IsSprintInputActive() || stamina->IsExhausted())
    {
        player->GetStateMachine()->ChangeState<PlayerStateNormal>();
        return;
    }

    const Vector3& moveDirection = player->GetMoveDirection();
    player->UpdateRotation(moveDirection);
    player->ApplyHorizontalMovement(moveDirection, player->config.runSpeed * player->config.sprintSpeedRate);
    player->UpdateFootstepEvents();
    stamina->Drain(player->config.sprintStaminaPerSecond);

    if (player->TryStartDodge())
        return;

    if (player->IsAimInputActive())
    {
        player->GetStateMachine()->ChangeState<PlayerStateAiming>();
        return;
    }

    player->TryStartJump();
}
