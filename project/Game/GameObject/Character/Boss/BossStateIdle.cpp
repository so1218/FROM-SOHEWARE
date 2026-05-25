#include "pch.h"
#include "BossStateIdle.h"
#include "BossStateJumpAttack.h"
#include "BossStateApproach.h"
#include "TimeManager.h"

using namespace FE;

void BossStateIdle::Enter(Boss* b)
{
    b->actionTimer_ = 0.0f; // Idleに入ったら必ずタイマーを初期化
}

void BossStateIdle::Update(Boss* b)
{
    float deltaTime = TimeManager::GetInstance()->GetDeltaTime();
    b->actionTimer_ += deltaTime;

    // プレイヤーの方を向く
    FE::Vector3 toPlayer = b->GetDirectionToPlayer();
    b->RotateTowards(toPlayer, b->rotateSpeed_ * 0.5f); // ゆっくり旋回

    // ゆっくり近づく
    b->GetTransform().translation_ += toPlayer * b->walkSpeed_;
    b->animation_->Play("bossWalk");

    // 2秒ごとに次の行動を判断
    if (b->actionTimer_ > 2.0f) 
    {
        float dist = b->GetDistanceToPlayer();
        b->actionTimer_ = 0.0f;

        if (dist > b->jumpAttackRange_) 
        {
            b->stateMachine_->ChangeState(BossStateJumpAttack::GetInstance());
        }
        else if (dist > b->meleeRange_)
        {
            b->stateMachine_->ChangeState(BossStateApproach::GetInstance());
        }
    }
}