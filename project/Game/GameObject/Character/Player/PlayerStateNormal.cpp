#include "pch.h"
#include "PlayerStateNormal.h"
#include "PlayerStateJump.h"
#include "Input.h"
#include "TimeManager.h"

using namespace FE;

void PlayerStateNormal::Update(Player* p)
{
    float deltaTime = TimeManager::GetInstance()->GetDeltaTime();

    // 常時重力を適用
    p->ApplyGravity(deltaTime);

    // 地形への着地位置補正
    p->SnapToGround();

    // 足場から外れた場合の落下遷移
    if (!p->IsGrounded())
    {
        p->GetStateMachine()->ChangeState(PlayerStateJump::GetInstance());
        return;
    }

    // 移動入力ベクトル
    Vector3 moveDir = p->GetMoveDirection();
    p->SetMoveDirection(moveDir);

    bool isMoving = (moveDir.Length() > 0.1f);

    // 状態の振り分け
    if (isMoving)
    {
        p->PlayAnimation("humanRun", true, p->config.runAnimSpeed, p->config.idleToRunBlendTime);
        p->UpdateRotation(moveDir);
        p->ApplyHorizontalMovement(moveDir, p->config.runSpeed);
    }
    else
    {
        p->PlayAnimation("humanIdle", true, p->config.idleAnimSpeed, p->config.runToIdleBlendTime);
    }

    // ジャンプ入力時に初速を設定して State 遷移
    if (Input::GetInstance().IsKeyTriggered(DIK_SPACE))
    {
        p->SetVelocityY(p->config.jumpInitialVelocity);
        p->GetStateMachine()->ChangeState(PlayerStateJump::GetInstance());
    }
}