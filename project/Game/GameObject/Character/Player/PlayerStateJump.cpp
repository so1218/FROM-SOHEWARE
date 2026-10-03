#include "pch.h"
#include "PlayerStateJump.h"
#include "PlayerStateNormal.h"
#include "TimeManager.h"

using namespace FE;

void PlayerStateJump::Enter(Player* player)
{
    player->PlayAnimation("humanJump", false, player->config.jumpAnimSpeed, player->config.jumpBlendTime);
}

void PlayerStateJump::Update(Player* player)
{
    float deltaTime = TimeManager::GetInstance()->GetDeltaTime();

    // 重力適用
    player->ApplyGravity(deltaTime);

    // 空中移動制御
    Vector3 moveDir = player->GetMoveDirection();
    player->UpdateRotation(moveDir);
    player->ApplyHorizontalMovement(moveDir, player->config.runSpeed * player->config.airControlRate);

    // 着地判定
    if (player->GetVelocityY() <= 0.0f && player->IsGrounded())
    {
        player->SnapToGround();
        player->GetStateMachine()->ChangeState(PlayerStateNormal::GetInstance());
    }
}