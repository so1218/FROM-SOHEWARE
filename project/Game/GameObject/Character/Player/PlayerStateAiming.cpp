#include "pch.h"
#include "PlayerStateAiming.h"
#include "PlayerStateNormal.h"
#include "TimeManager.h"
#include "Input.h"

using namespace FE;

void PlayerStateAiming::Enter(Player* player)
{
    // エイム開始アニメーション再生
    player->PlayAnimation("humanPistolIdle", true, 0.5f, 0.1f);

    // カメラをエイムモードにする
    player->GetFollowCamera()->SetAiming(true);
}

void PlayerStateAiming::Update(Player* player)
{
    float deltaTime = TimeManager::GetInstance()->GetDeltaTime();
    auto& input = Input::GetInstance();

    player->ApplyGravity(deltaTime);
    player->SnapToGround();

    bool isAimingInput = input.IsKeyPressed(DIK_K) || input.IsControllerButtonPressed(0, Input::ButtonLT);

    // どちらの入力も無い場合、通常状態に戻る
    if (!isAimingInput)
    {
        player->GetStateMachine()->ChangeState(PlayerStateNormal::GetInstance());
        return;
    }

    // 回転処理：エイム中は常にカメラの正面に体を向ける
    player->UpdateAimRotation();

    // 移動処理とアニメーション分岐
    Vector3 moveDir = player->GetMoveDirection();
    bool isMoving = (moveDir.Length() > 0.1f);

    if (isMoving)
    {
        // 位置移動
        player->ApplyHorizontalMovement(moveDir, player->config.aimMoveSpeed);

        // ---------------------------------------------------------
        // プレイヤーのローカル方向に対する移動向きの判定
        // ---------------------------------------------------------
        Quaternion playerRot = player->GetTransform().rotationQuaternion_;
        Vector3 playerForward = playerRot.RotateVector({ 0.0f, 0.0f, 1.0f }); // プレイヤーの正面
        Vector3 playerRight = playerRot.RotateVector({ 1.0f, 0.0f, 0.0f }); // プレイヤーの右方向

        // 移動ベクトルとの内積を計算 
        float dotForward = moveDir.Dot(playerForward);
        float dotRight = moveDir.Dot(playerRight);

        // 縦方向の入力と横方向の入力のどちらが大きいかで判定
        if (std::abs(dotForward) >= std::abs(dotRight))
        {
            if (dotForward > 0.0f)
            {
                // 前進
                player->PlayAnimation("humanPistolWalkForward", true, 0.8f, 0.1f);
            }
            else
            {
                // 後退
                player->PlayAnimation("humanPistolWalkBackward", true, 0.8f, 0.1f);
            }
        }
        else
        {
            if (dotRight > 0.0f)
            {
                // 右移動
                player->PlayAnimation("humanPistolWalkRight", true, 0.8f, 0.1f);
            }
            else
            {
                // 左移動
                player->PlayAnimation("humanPistolWalkLeft", true, 0.8f, 0.1f);
            }
        }
    }
    else
    {
        // 移動していない時はエイム待機アニメーション
        player->PlayAnimation("humanPistolIdle", true, 0.5f, 0.1f);
    }

    // 毎フレームのレティクル収束計算
    player->UpdateReticle(deltaTime, isMoving, true);

    // 射撃処理
    if (input.IsKeyTriggered(DIK_J) || input.IsControllerButtonTriggered(0, Input::ButtonRT))
    {
        player->FireWeapon();
        // 射撃の跳ね上がりでレティクルを開かせる
        player->OnShootRecoil();

        input.StartVibration(0, 0.6f, 0.6f, 0.25f);
    }
}

void PlayerStateAiming::Exit(Player* player)
{
    // 通常カメラモードに戻す
    player->GetFollowCamera()->SetAiming(false);
    // エイム解除時にフェードアウトへ向けてリセット
    player->UpdateReticle(0.0f, false, false);

    player->ResetReticle();
}