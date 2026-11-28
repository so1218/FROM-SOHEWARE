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
    Crawl,
    SwitchAmmo,

    Pause,
    Confirm,
    Cancel,
    SkipTutorial,
    Reply,
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
// マウスで視点を動かした量
FS::Vector2 GetMouseLookDelta();
// true の間は、カーソルを隠して閉じ込め、マウスで視点と銃を操作
void SetCursorLocked(bool isLocked);
// 閉じ込める範囲
void SetCursorLockArea(const FS::Vector2& min, const FS::Vector2& size);
// メニューの上下の入力
float GetMenuVertical();
// メニューの左右の入力
float GetMenuHorizontal();

// 最後に触ったのがパッドか
bool IsUsingController();

void StartVibration(float leftMotorSpeed, float rightMotorSpeed, float durationSeconds);

}
