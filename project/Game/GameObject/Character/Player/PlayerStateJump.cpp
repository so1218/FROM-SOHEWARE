#include "pch.h"
#include "PlayerStateJump.h"
#include "PlayerStateNormal.h"

using namespace FS;

void PlayerStateJump::Enter(Player* player)
{
    player->PlayAnimation("humanJump", false, player->config.jumpAnimSpeed, player->config.jumpBlendTime);
}

void PlayerStateJump::Update(Player* player)
{
    player->ApplyGravity();

    // 空中でも少しだけ動ける
    const Vector3& moveDirection = player->GetMoveDirection();
    player->UpdateRotation(moveDirection);
    player->ApplyHorizontalMovement(moveDirection, player->config.runSpeed * player->config.airControlRate);

    if (player->GetVelocityY() <= 0.0f && player->IsGrounded())
    {
        player->SnapToGround();
        player->GetStateMachine()->ChangeState<PlayerStateNormal>();
    }
}
