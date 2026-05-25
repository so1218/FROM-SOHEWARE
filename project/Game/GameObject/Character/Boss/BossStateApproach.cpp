#include "pch.h"
#include "BossStateApproach.h"
#include "BossStateIdle.h"

void BossStateApproach::Update(Boss* b)
{
    FE::Vector3 toPlayer = b->GetDirectionToPlayer();
    b->RotateTowards(toPlayer, b->rotateSpeed_); // 素早く旋回

    // 走って近づく
    b->GetTransform().translation_ += toPlayer * b->runSpeed_;
    b->animation_->Play("bossRun");

    // 近接範囲内に入ったら
    if (b->GetDistanceToPlayer() <= b->meleeRange_)
    {
        // 本来はここで攻撃ステートへ。今は一旦Idleに戻る
        b->stateMachine_->ChangeState(BossStateIdle::GetInstance());
    }
}