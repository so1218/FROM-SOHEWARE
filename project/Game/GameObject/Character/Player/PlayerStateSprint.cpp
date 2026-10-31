#include "pch.h"
#include "PlayerStateSprint.h"
#include "PlayerStateNormal.h"
#include "PlayerStateJump.h"
#include "PlayerStateAiming.h"
#include "PlayerStateDodge.h"
#include "PlayerStamina.h"
#include "Input.h"

using namespace FS;

void PlayerStateSprint::Enter(Player* player)
{
    player->PlayAnimation("humanRun", true, player->config.sprintAnimSpeed, player->config.idleToRunBlendTime);
}

void PlayerStateSprint::Update(Player* player)
{
    auto& input = Input::GetInstance();
    PlayerStamina* stamina = player->GetStamina();

    player->ApplyGravity();
    player->SnapToGround();

    if (!player->IsGrounded())
    {
        player->GetStateMachine()->ChangeState(PlayerStateJump::GetInstance());
        return;
    }

    Vector3 moveDir = player->GetMoveDirection();
    player->SetMoveDirection(moveDir);
    const bool isMoving = (moveDir.Length() > 0.1f);

    // 止まった、ボタンを離した、息切れした
    if (!isMoving || !player->IsSprintInputActive() || stamina->IsExhausted())
    {
        player->GetStateMachine()->ChangeState(PlayerStateNormal::GetInstance());
        return;
    }

    player->UpdateRotation(moveDir);
    player->ApplyHorizontalMovement(moveDir, player->config.runSpeed * player->config.sprintSpeedRate);
    player->UpdateFootstepEvents();
    stamina->Drain(player->config.sprintStaminaPerSecond);

    if (player->IsDodgeInputTriggered() && stamina->TryConsume(player->config.dodgeStaminaCost))
    {
        player->GetStateMachine()->ChangeState(PlayerStateDodge::GetInstance());
        return;
    }

    if (input.IsKeyPressed(DIK_K) || input.IsControllerButtonPressed(0, Input::ButtonLT))
    {
        player->GetStateMachine()->ChangeState(PlayerStateAiming::GetInstance());
        return;
    }

    if (input.IsKeyTriggered(DIK_SPACE) || input.IsControllerButtonTriggered(0, Input::ButtonA))
    {
        player->SetVelocityY(player->config.jumpInitialVelocity);
        player->GetStateMachine()->ChangeState(PlayerStateJump::GetInstance());
    }
}
