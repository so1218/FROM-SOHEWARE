#pragma once

#define NOMINMAX 
#include <windows.h>
#include <cassert>
#include <cstring>
#define DIRECTINPUT_VERSION    0x0800// DirectInputのバージョン指定
#include <dinput.h>
#include <Xinput.h>

#include "Vector2.h"

class Input
{
public:
    Input();
    ~Input();

    enum StickType 
    {
        LeftStick,
        RightStick
    };

    // Xboxコントローラーのボタン列挙
    enum PadButton 
    {
        ButtonUp = XINPUT_GAMEPAD_DPAD_UP,
        ButtonDown = XINPUT_GAMEPAD_DPAD_DOWN,
        ButtonLeft = XINPUT_GAMEPAD_DPAD_LEFT,
        ButtonRight = XINPUT_GAMEPAD_DPAD_RIGHT,
        ButtonMenu = XINPUT_GAMEPAD_START,
        ButtonPhoto = XINPUT_GAMEPAD_BACK,
        ButtonLT = XINPUT_GAMEPAD_LEFT_THUMB,
        ButtonRT = XINPUT_GAMEPAD_RIGHT_THUMB,
        ButtonLB = XINPUT_GAMEPAD_LEFT_SHOULDER,
        ButtonRB = XINPUT_GAMEPAD_RIGHT_SHOULDER,
        ButtonA = XINPUT_GAMEPAD_A,
        ButtonB = XINPUT_GAMEPAD_B,
        ButtonX = XINPUT_GAMEPAD_X,
        ButtonY = XINPUT_GAMEPAD_Y,
    };

    enum MouseButton
    {
        Left = 0,
        Right = 1,
        Middle = 2,
    };

    static void Initialize(HINSTANCE hInstance, HWND hwnd);
    static void Update(); // 入力状態の更新
    // マウスホイールのスクロール量を取得する
    static int GetMouseWheelDelta();
    static Vector2  GetMousePosition();
    static int GetMouseX();        
    static int GetMouseY();
    static bool IsKeyTriggered(BYTE key); // キーが押された瞬間
    static bool IsKeyPressed(BYTE key);   // キーが常に押されてるかどうか
    static bool IsKeyReleased(BYTE key);   // キーを離した瞬間
    static bool IsKeyUp(BYTE key);   // キーが常に押されてないかどうか
    static bool IsMouseButtonTriggered(DWORD button);// マウスキーが押された瞬間
    static bool IsMouseButtonPressed(DWORD button);// マウスキーが常に押されてるかどうか
    static bool IsMouseButtonIsKeyReleased(DWORD button);// マウスキーを離した瞬間
    static bool IsMouseButtonUp(DWORD button);// マウスキーが常に押されてないかどうか

    static bool IsControllerConnected(int controllerId);
    static bool IsControllerButtonPressed(int controllerId, WORD button);
    static bool IsControllerButtonTriggered(int controllerId, WORD button);
    static bool IsControllerButtonReleased(int controllerId, WORD button);
    static SHORT GetLeftTrigger(int controllerId);
    static SHORT GetRightTrigger(int controllerId);
    static SHORT GetLeftStickX(int controllerId);
    static SHORT GetLeftStickY(int controllerId);
    static SHORT GetRightStickX(int controllerId);
    static SHORT GetRightStickY(int controllerId);
    static bool IsLeftOnStick(int controllerId, StickType stickType);
    static bool IsRightOnStick(int controllerId, StickType stickType);
    static bool IsUpOnStick(int controllerId, StickType stickType);
    static bool IsDownOnStick(int controllerId, StickType stickType);
    static bool IsTriggerOnStick(int controllerId, StickType stickType);
    static void VibrateController(int controllerId, WORD leftMotor, WORD rightMotor);
    static void UpdateController();

    static const DIMOUSESTATE& GetMouseState() { return mouseState_; }
    static const DIMOUSESTATE& GetPrevMouseState() { return preMouseState_; }

private:
    static DIMOUSESTATE mouseState_;           // 現在のマウスの状態
    static DIMOUSESTATE preMouseState_;        // 前回のマウスの状態

    static IDirectInput8* directInput_;
    static IDirectInputDevice8* keyboard_;
    static BYTE keys_[256];
    static BYTE preKeys_[256];
    static IDirectInputDevice8* mouse_;     
    static POINT mousePosition_;         
    static HWND hwnd_;

    static XINPUT_STATE controllerStates_[4];
    static XINPUT_STATE prevControllerStates_[4];
    static bool controllerConnected_[4];
};

