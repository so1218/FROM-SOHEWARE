#include "pch.h"
#include "PlayerStateAiming.h"
#include "PlayerStateNormal.h"
#include "PlayerReticle.h"

using namespace FS;

void PlayerStateAiming::Enter(Player* player)
{
    player->PlayAnimation("humanPistolIdle", true, 0.5f, 0.1f);
    player->GetFollowCamera()->SetAiming(true);
}

void PlayerStateAiming::Update(Player* player)
{
    player->ApplyGravity();
    player->SnapToGround();

    if (!player->IsAimInputActive())
    {
        player->GetStateMachine()->ChangeState<PlayerStateNormal>();
        return;
    }

    if (player->TryStartDodge())
        return;

    // エイム中は常にカメラの正面を向くので、移動は体の向きに対して前後左右のどれかで歩き方を変える
    player->UpdateAimRotation();

    if (player->IsMoving())
    {
        const Vector3& moveDirection = player->GetMoveDirection();
        player->ApplyHorizontalMovement(moveDirection, player->config.aimMoveSpeed);

        const Quaternion& rotation = player->GetTransform().rotation_;
        const float forwardAmount = moveDirection.Dot(rotation.RotateVector({ 0.0f, 0.0f, 1.0f }));
        const float rightAmount = moveDirection.Dot(rotation.RotateVector({ 1.0f, 0.0f, 0.0f }));

        const char* animationName = nullptr;
        if (std::abs(forwardAmount) >= std::abs(rightAmount))
        {
            animationName = forwardAmount > 0.0f ? "humanPistolWalkForward" : "humanPistolWalkBackward";
        }
        else
        {
            animationName = rightAmount > 0.0f ? "humanPistolWalkRight" : "humanPistolWalkLeft";
        }
        player->PlayAnimation(animationName, true, 0.8f, 0.1f);
    }
    else
    {
        player->PlayAnimation("humanPistolIdle", true, 0.5f, 0.1f);
    }

    if (player->IsShootInputTriggered())
    {
        player->FireWeapon();
    }
}

void PlayerStateAiming::Exit(Player* player)
{
    player->GetFollowCamera()->SetAiming(false);

    if (player->GetReticle())
    {
        player->GetReticle()->Reset();
    }
}
