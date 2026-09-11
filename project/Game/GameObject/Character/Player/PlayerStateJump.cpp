#include "pch.h"
#include "PlayerStateJump.h"
#include "PlayerStateNormal.h"
#include "TimeManager.h"

using namespace FE;

void PlayerStateJump::Enter(Player* p)
{
    // 初速設定は遷移元で行うため、アニメーション再生のみ
    p->PlayAnimation("humanJump", false, p->config.jumpAnimSpeed, p->config.jumpBlendTime);
}

void PlayerStateJump::Update(Player* p)
{
    float deltaTime = TimeManager::GetInstance()->GetDeltaTime();

    // 重力適用
    p->ApplyGravity(deltaTime);

    // 空中移動制御
    Vector3 moveDir = p->GetMoveDirection();
    p->UpdateRotation(moveDir);
    p->ApplyHorizontalMovement(moveDir, p->config.runSpeed * p->config.airControlRate);

    // 着地判定
    if (p->GetVelocityY() <= 0.0f && p->IsGrounded())
    {
        p->SnapToGround();
        p->GetStateMachine()->ChangeState(PlayerStateNormal::GetInstance());
    }
}