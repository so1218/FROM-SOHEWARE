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
        // 移動キーが入力されている時は走る
        p->moveSpeed_ = p->runSpeed_;
        p->animationModel_->Play("humanRun");
    }
    else
    {
        // 入力がない時は止まる
        p->moveSpeed_ = 0.0f;
        p->animationModel_->Play("humanIdle");
    }

    // 実際の座標・回転更新処理
    p->Move();
}