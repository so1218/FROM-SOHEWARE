#pragma once

#define NOMINMAX 
#include <windows.h>
#include <cassert>
#include <cstring>
#define DIRECTINPUT_VERSION    0x0800// DirectInputのバージョン指定
#include <dinput.h>
#include <Xinput.h>

#define STICK_THRESHOLD 0x4000

#include "Vector2.h"

class Input
{
public:
    static Input& GetInstance();

    // コピーと代入を禁止
    Input(const Input&) = delete;
    Input& operator=(const Input&) = delete;

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
        ButtonL3 = XINPUT_GAMEPAD_LEFT_THUMB,
        ButtonR3 = XINPUT_GAMEPAD_RIGHT_THUMB,
        ButtonLB = XINPUT_GAMEPAD_LEFT_SHOULDER,
        ButtonRB = XINPUT_GAMEPAD_RIGHT_SHOULDER,
        ButtonA = XINPUT_GAMEPAD_A,
        ButtonB = XINPUT_GAMEPAD_B,
        ButtonX = XINPUT_GAMEPAD_X,
        ButtonY = XINPUT_GAMEPAD_Y,
        ButtonLT = 0x10000,
        ButtonRT = 0x20000,
    };

    enum MouseButton
    {
        Left = 0,
        Right = 1,
        Middle = 2,
    };

    void Initialize(HINSTANCE hInstance, HWND hwnd);
    void Update();
    void Finalize();

    // マウス関連
    int GetMouseWheelDelta();
    Vector2  GetMousePosition();
    int GetMouseX();
    int GetMouseY();
    bool IsMouseButtonTriggered(DWORD button);// マウスキーが押された瞬間
    bool IsMouseButtonPressed(DWORD button);// マウスキーが常に押されてるかどうか
    bool IsMouseButtonIsKeyReleased(DWORD button);// マウスキーを離した瞬間
    bool IsMouseButtonUp(DWORD button);// マウスキーが常に押されてないかどうか

    // キーボード関連
    bool IsKeyTriggered(BYTE key); // キーが押された瞬間
    bool IsKeyPressed(BYTE key);   // キーが常に押されてるかどうか
    bool IsKeyReleased(BYTE key);   // キーを離した瞬間
    bool IsKeyUp(BYTE key);   // キーが常に押されてないかどうか

    // コントローラー関連
    bool IsControllerConnected(int controllerId);
    bool IsControllerButtonPressed(int controllerId, int button);
    bool IsControllerButtonTriggered(int controllerId, int button);
    bool IsControllerButtonReleased(int controllerId, int button);
    SHORT GetLeftTrigger(int controllerId);
    SHORT GetRightTrigger(int controllerId);
    SHORT GetLeftStickX(int controllerId);
    SHORT GetLeftStickY(int controllerId);
    SHORT GetRightStickX(int controllerId);
    SHORT GetRightStickY(int controllerId);
    bool IsLeftOnStick(int controllerId, StickType stickType);
    bool IsRightOnStick(int controllerId, StickType stickType);
    bool IsUpOnStick(int controllerId, StickType stickType);
    bool IsDownOnStick(int controllerId, StickType stickType);
    bool IsTriggerOnStick(int controllerId, StickType stickType);
    void VibrateController(int controllerId, float leftMotorSpeed, float rightMotorSpeed);
    void StartVibration(int controllerId, float leftMotorSpeed, float rightMotorSpeed, float durationSeconds);
    void UpdateController();

    const DIMOUSESTATE& GetMouseState() { return mouseState_; }
    const DIMOUSESTATE& GetPrevMouseState() { return preMouseState_; }

private:
    Input();
    ~Input();

    // DirectInput
    IDirectInput8* directInput_;
    HWND hwnd_;

    // キーボード
    IDirectInputDevice8* keyboard_;
    BYTE keys_[256];
    BYTE preKeys_[256];

    // マウス
    IDirectInputDevice8* mouse_;
    DIMOUSESTATE mouseState_;
    DIMOUSESTATE preMouseState_;
    POINT mousePosition_;

    // コントローラー
    XINPUT_STATE controllerStates_[4];
    XINPUT_STATE prevControllerStates_[4];
    bool controllerConnected_[4];
    float vibrationTimers[4];
};