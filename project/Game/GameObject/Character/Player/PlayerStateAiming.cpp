#include "pch.h"
#include "PlayerStateAiming.h"
#include "PlayerStateNormal.h"
#include "TimeManager.h"
#include "Input.h"

using namespace FE;

void PlayerStateAiming::Enter(Player* player)
{
    // エイム開始アニメーション再生
    player->PlayAnimation("humanPistolIdle", true, 1.0f, 0.1f);

    // カメラをエイムモードにする
    player->GetFollowCamera()->SetAiming(true);
}

void PlayerStateAiming::Update(Player* player)
{
    float deltaTime = TimeManager::GetInstance()->GetDeltaTime();
    auto& input = Input::GetInstance();

    player->ApplyGravity(deltaTime);
    player->SnapToGround();

    // Qキーを離したら通常状態に戻る
    if (!input.IsKeyPressed(DIK_Q))
    {
        player->GetStateMachine()->ChangeState(PlayerStateNormal::GetInstance());
        return;
    }

    // 回転処理：エイム中は進行方向ではなくカメラの正面に常に体を向ける
    player->UpdateAimRotation();

    // 移動処理
    Vector3 moveDir = player->GetMoveDirection();
    if (moveDir.Length() > 0.1f)
    {
        player->ApplyHorizontalMovement(moveDir, player->config.aimMoveSpeed);
    }

    // 射撃処理
    if (input.IsKeyTriggered(DIK_E))
    {
        player->FireWeapon();
    }
}

void PlayerStateAiming::Exit(Player* player)
{
    // 通常カメラモードに戻す
    player->GetFollowCamera()->SetAiming(false);
}