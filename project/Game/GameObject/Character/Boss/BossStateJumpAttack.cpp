#include "pch.h"
#include "BossStateJumpAttack.h"
#include "TimeManager.h"
#include "BossStateIdle.h"

using namespace FE;

void BossStateJumpAttack::Enter(Boss* b)
{
    b->actionTimer_ = 0.0f;
    // 飛び込む方向を固定（ジャンプ中に追従しすぎないようにする）
    b->targetDir_ = b->GetDirectionToPlayer();
    b->animation_->Play("bossJumpAttack", false);
}

void BossStateJumpAttack::Update(Boss* b)
{
    float deltaTime = TimeManager::GetInstance()->GetDeltaTime();
    b->actionTimer_ += deltaTime;

    // 高速移動
    b->GetTransform().translation_ += b->targetDir_ * b->jumpAttackSpeed_;

    // 1秒経ったら着地
    if (b->actionTimer_ > 1.0f) 
    {
        b->stateMachine_->ChangeState(BossStateIdle::GetInstance());
    }
}