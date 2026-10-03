#include "pch.h"
#include "PlayerStateNormal.h"
#include "PlayerStateJump.h"
#include "PlayerStateAiming.h"
#include "Input.h"
#include "TimeManager.h"
#include "AudioPlayer.h"

using namespace FE;

void PlayerStateNormal::Update(Player* player)
{
    float deltaTime = TimeManager::GetInstance()->GetDeltaTime();

    // 常時重力を適用
    player->ApplyGravity(deltaTime);

    // 地形への着地位置補正
    player->SnapToGround();

    // 足場から外れた場合の落下遷移
    if (!player->IsGrounded())
    {
        player->GetStateMachine()->ChangeState(PlayerStateJump::GetInstance());
        return;
    }

    // 移動入力ベクトル
    Vector3 moveDir = player->GetMoveDirection();
    player->SetMoveDirection(moveDir);

    bool isMoving = (moveDir.Length() > 0.1f);

    // 状態の振り分け
    if (isMoving)
    {
        player->PlayAnimation("humanRun", true, player->config.runAnimSpeed, player->config.idleToRunBlendTime);
        player->UpdateRotation(moveDir);
        player->ApplyHorizontalMovement(moveDir, player->config.runSpeed);

        // 移動中のみ足音のタイミング判定を実行
        player->UpdateFootstepEvents();
    }
    else
    {
        player->PlayAnimation("humanIdle", true, player->config.idleAnimSpeed, player->config.runToIdleBlendTime);

        // 止まったら足音の進捗率をリセット
        player->ResetFootstepState();
    }

    // Kキーが押されていたらエイム状態に遷移
    if (Input::GetInstance().IsKeyPressed(DIK_K) || Input::GetInstance().IsControllerButtonPressed(0, Input::ButtonLT))
    {
        player->GetStateMachine()->ChangeState(PlayerStateAiming::GetInstance());
        return;
    }

    // ジャンプ入力時に初速を設定して State 遷移
    if (Input::GetInstance().IsKeyTriggered(DIK_SPACE) || Input::GetInstance().IsControllerButtonTriggered(0, Input::ButtonA))
    {
        player->SetVelocityY(player->config.jumpInitialVelocity);
        player->GetStateMachine()->ChangeState(PlayerStateJump::GetInstance());
    }
}