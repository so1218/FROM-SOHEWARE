#pragma once
#include "Vector2.h"

// ゲームの操作
enum class GameAction
{
    Jump,

    Pause,
    Confirm,
    Cancel,
};

// ゲームの入力
namespace GameInput
{

// 割り当てのどれかを押している間
bool IsPressed(GameAction action);
// 割り当てのどれかを押した瞬間
bool IsTriggered(GameAction action);
// 割り当てをすべて離した瞬間
bool IsReleased(GameAction action);

// 移動の入力 
FS::Vector2 GetMoveAxis();
// 視点の入力
FS::Vector2 GetLookAxis(float stickDeadzone);
// メニューの上下の入力
float GetMenuVertical();

// 最後に触ったのがパッドか
bool IsUsingController();

void StartVibration(float leftMotorSpeed, float rightMotorSpeed, float durationSeconds);

}
