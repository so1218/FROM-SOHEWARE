#pragma once
#include "Vector2.h"

// ゲームの操作
enum class GameAction
{
    Sprint,
    Dodge,
    Aim,
    Jump,
    Reload,
    Shoot,

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

// 移動の入力 (WASD・左スティック)。長さが入力の強さ (0 ~ 1)
FS::Vector2 GetMoveAxis();
// 視点の入力 (矢印キー・右スティック)。各軸 -1 ~ 1。stickDeadzone はスティックを倒した量 (0 ~ 1)
FS::Vector2 GetLookAxis(float stickDeadzone);
// メニューの上下の入力。上が +1、下が -1。押しっぱなしで進めるときは RepeatInput に渡す
float GetMenuVertical();

// 最後に触ったのがパッドか
bool IsUsingController();

void StartVibration(float leftMotorSpeed, float rightMotorSpeed, float durationSeconds);

}
