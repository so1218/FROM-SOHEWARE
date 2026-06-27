#include "pch.h"
#include "PlayerStateNormal.h"
#include "PlayerStateRoll.h"
#include "Input.h"
#include "TimeManager.h"

using namespace FE;

void PlayerStateNormal::Update(Player* p)
{
    auto& input = Input::GetInstance();
    float deltaTime = TimeManager::GetInstance()->GetDeltaTime();

    // --- 1. スタミナの自動回復 ---
    if (p->stamina_ < p->maxStamina_) 
    {
        p->stamina_ += p->staminaRecoveryRate_ * deltaTime;
        if (p->stamina_ > p->maxStamina_) p->stamina_ = p->maxStamina_;
    }

    // --- 2. ローリングへの遷移判定 ---
    // 例: スペースキー or コントローラーのAボタン(トリガー)
    if (input.IsKeyTriggered(DIK_SPACE) || input.IsControllerButtonTriggered(0, XINPUT_GAMEPAD_B)) {
        if (p->stamina_ >= p->rollStaminaCost_)
        { 
            p->GetStateMachine()->ChangeState(PlayerStateRoll::GetInstance());
            return; // 遷移したらこのフレームの処理は終了
        }
    }

    // --- 3. 移動とダッシュの処理 ---
    p->moveDirection_ = p->GetMoveDirection();
    bool isMoving = (p->moveDirection_.Length() > 0.1f);

    // Aボタン(または特定のボタン)長押し ＆ 移動中 ＆ スタミナありならダッシュ
    bool isDashing = isMoving && (input.IsKeyPressed(DIK_LSHIFT) || input.IsControllerButtonPressed(0, XINPUT_GAMEPAD_A)) && (p->stamina_ > 0.0f);

    if (isDashing)
    {
        p->moveSpeed_ = p->dashSpeed_;
        p->stamina_ -= p->dashStaminaCost_ * deltaTime;

    }
    else if (isMoving)
    {
        p->moveSpeed_ = p->runSpeed_;
    }
    else
    {
    }

    if (input.IsKeyTriggered(DIK_SPACE) || input.IsControllerButtonTriggered(0, input.ButtonA))
    {
        if (p->stamina_ >= p->rollStaminaCost_) 
        { 
            p->GetStateMachine()->ChangeState(PlayerStateRoll::GetInstance());
            return;
        }
    }

    // 実際の座標更新処理
    p->Move();
}