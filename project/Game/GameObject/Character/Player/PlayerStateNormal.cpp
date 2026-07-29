#include "pch.h"
#include "PlayerStateNormal.h"
#include "Input.h"
#include "TimeManager.h"

using namespace FE;

void PlayerStateNormal::Update(Player* p)
{
    // 移動入力の取得
    p->moveDirection_ = p->GetMoveDirection();

    // ベクトルの長さで移動中かどうかを判定
    bool isMoving = (p->moveDirection_.Length() > 0.1f);

    if (isMoving)
    {
        p->moveSpeed_ = p->runSpeed_;

        // 走りアニメーションの再生
        p->animationModel_->Play(
            "humanRun",
            true,
            p->runAnimSpeed_,
            p->idleToRunBlendTime_
        );
    }
    else
    {
        p->moveSpeed_ = 0.0f;

        // 待機アニメーションの再生
        p->animationModel_->Play(
            "humanIdle",
            true,
            p->idleAnimSpeed_,
            p->runToIdleBlendTime_
        );
    }

    // 実際の座標・回転更新処理
    p->Move();
}