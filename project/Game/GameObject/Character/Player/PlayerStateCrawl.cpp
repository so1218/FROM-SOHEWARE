#include "pch.h"
#include "PlayerStateCrawl.h"
#include "PlayerStateNormal.h"
#include "PlayerStateJump.h"
#include "PlayerStateAiming.h"
#include "PlayerStateSprint.h"
#include "PlayerStamina.h"
#include "AnimationManager.h"

using namespace FS;

namespace
{

const char* SelectAnimation(const char* crawlName, const char* fallbackName)
{
    return AnimationManager::GetInstance()->Get(crawlName) ? crawlName : fallbackName;
}

}

void PlayerStateCrawl::Enter(Player* player)
{
    // ダッシュから這うに移ったら、ダッシュは取り消す
    player->CancelSprintInput();
}

void PlayerStateCrawl::Update(Player* player)
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
        player->PlayAnimation(SelectAnimation("humanCrawl", "humanRun"), true, player->config.crawlWalkAnimSpeed, player->config.crawlBlendTime);
        player->UpdateRotation(moveDirection);
        player->ApplyHorizontalMovement(moveDirection, player->config.crawlSpeed);
    }
    else
    {
        player->PlayAnimation(SelectAnimation("humanCrawlIdle", "humanIdle"), true, player->config.crawlIdleAnimSpeed, player->config.crawlBlendTime);
    }
    player->ResetFootstepState();

    if (player->IsCrawlInputTriggered())
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
