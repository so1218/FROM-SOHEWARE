#include "Input.h"
#include "TimeManager.h"
#include "MathUtils.h"

#define STICK_THRESHOLD 0x4000

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "Xinput.lib")

Input& Input::GetInstance()
{
    static Input instance;
    return instance;
}

Input::Input() 
{
    directInput_ = nullptr;
    keyboard_ = nullptr;
    mouse_ = nullptr;
    hwnd_ = nullptr;

    ZeroMemory(keys_, sizeof(keys_));
    ZeroMemory(preKeys_, sizeof(preKeys_));
    ZeroMemory(&mouseState_, sizeof(mouseState_));
    ZeroMemory(&preMouseState_, sizeof(preMouseState_));
    ZeroMemory(&mousePosition_, sizeof(mousePosition_));

    for (int i = 0; i < 4; i++)
    {
        controllerConnected_[i] = false;
        vibrationTimers[i] = 0;
        ZeroMemory(&controllerStates_[i], sizeof(XINPUT_STATE));
        ZeroMemory(&prevControllerStates_[i], sizeof(XINPUT_STATE));
    }
}

Input::~Input() 
{
    if (keyboard_)
    {
        keyboard_->Unacquire();
        keyboard_->Release();
    }
    if (mouse_)
    {
        mouse_->Unacquire();
        mouse_->Release();
    }
    if (directInput_) 
    {
        directInput_->Release();
    }
}

void Input::Initialize(HINSTANCE hInstance, HWND hwnd)
{
    hwnd_ = hwnd;

    // DirectInput
    HRESULT hr = DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&directInput_, nullptr);
    assert(SUCCEEDED(hr));

    // キーボード
    hr = directInput_->CreateDevice(GUID_SysKeyboard, &keyboard_, NULL);
    assert(SUCCEEDED(hr));

    hr = keyboard_->SetDataFormat(&c_dfDIKeyboard);
    assert(SUCCEEDED(hr));

    hr = keyboard_->SetCooperativeLevel(hwnd_, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
    assert(SUCCEEDED(hr));

    // マウス
    hr = directInput_->CreateDevice(GUID_SysMouse, &mouse_, NULL);
    assert(SUCCEEDED(hr));

    hr = mouse_->SetDataFormat(&c_dfDIMouse);
    assert(SUCCEEDED(hr));

    hr = mouse_->SetCooperativeLevel(hwnd_, DISCL_NONEXCLUSIVE | DISCL_BACKGROUND);
    assert(SUCCEEDED(hr));
}

void Input::Update()
{
    // 前回の状態を保存
    memcpy(preKeys_, keys_, sizeof(keys_));
    preMouseState_ = mouseState_; // structは直接代入でOK

    // デバイスの制御を取得
    if (keyboard_) 
    {
        keyboard_->Acquire();
        keyboard_->GetDeviceState(sizeof(keys_), keys_);
    }
    if (mouse_) 
    {
        mouse_->Acquire();
        mouse_->GetDeviceState(sizeof(DIMOUSESTATE), &mouseState_);
    }

    // マウスカーソル位置の更新
    GetCursorPos(&mousePosition_);
    ScreenToClient(hwnd_, &mousePosition_);

    // コントローラーの状態を更新
    UpdateController();
}

void Input::UpdateController()
{
    for (int i = 0; i < 4; ++i)
    {
        // 現在の状態を前回の状態として保存
        prevControllerStates_[i] = controllerStates_[i];

        // 新しい状態を取得
        DWORD result = XInputGetState(i, &controllerStates_[i]);

        // 接続状態を更新
        controllerConnected_[i] = (result == ERROR_SUCCESS);
    }

    for (int i = 0; i < 4; ++i)
    {
        // タイマーが作動中の場合
        if (vibrationTimers[i] > 0.0f)
        {
            vibrationTimers[i] -= TimeManager::GetInstance()->GetDeltaTime(); // 経過時間を引く

            // タイマーが0以下になったら
            if (vibrationTimers[i] <= 0.0f)
            {
                vibrationTimers[i] = 0.0f;
                VibrateController(i, 0, 0); // 振動を停止
            }
        }
    }
}

int Input::GetMouseWheelDelta()
{
    return mouseState_.lZ; 
}

Vector2 Input::GetMousePosition()
{
    return Vector2
    {
       static_cast<float>(mousePosition_.x),
       static_cast<float>(mousePosition_.y)
    };
}

int Input::GetMouseX()
{
    return static_cast<int>(mousePosition_.x);
}

int Input::GetMouseY()
{
    return static_cast<int>(mousePosition_.y);
}

bool Input::IsKeyTriggered(BYTE key)
{
    return (keys_[key] & 0x80) && !(preKeys_[key] & 0x80);
}
bool Input::IsKeyPressed(BYTE key)
{
    return keys_[key] & 0x80;
}
bool Input::IsKeyReleased(BYTE key)
{
    return !(keys_[key] & 0x80) && (preKeys_[key] & 0x80);
}
bool Input::IsKeyUp(BYTE key)
{
    return !(keys_[key] & 0x80);
}
bool Input::IsMouseButtonTriggered(DWORD button)
{
    return (mouseState_.rgbButtons[button] & 0x80) && !(preMouseState_.rgbButtons[button] & 0x80);
}
bool Input::IsMouseButtonPressed(DWORD button) 
{
    return (mouseState_.rgbButtons[button] & 0x80);
}
bool Input::IsMouseButtonIsKeyReleased(DWORD button) 
{
    return !(mouseState_.rgbButtons[button] & 0x80) && (preMouseState_.rgbButtons[button] & 0x80);
}
bool Input::IsMouseButtonUp(DWORD button) 
{
    return !(preMouseState_.rgbButtons[button] & 0x80);
}

bool Input::IsControllerConnected(int controllerId)
{
    return (XInputGetState(controllerId, &controllerStates_[controllerId]) == ERROR_SUCCESS);
}

bool Input::IsControllerButtonPressed(int controllerId, WORD button)
{
    return (controllerStates_[controllerId].Gamepad.wButtons & button) == button;
}

bool Input::IsControllerButtonTriggered(int controllerId, WORD button)
{
    return  ((controllerStates_[controllerId].Gamepad.wButtons & button) == button) &&
        !((prevControllerStates_[controllerId].Gamepad.wButtons & button) == button);
}

bool Input::IsControllerButtonReleased(int controllerId, WORD button)
{
    return  !((controllerStates_[controllerId].Gamepad.wButtons & button) == button) &&
        ((prevControllerStates_[controllerId].Gamepad.wButtons & button) == button);
}

SHORT Input::GetLeftTrigger(int controllerId)
{
    return controllerStates_[controllerId].Gamepad.bLeftTrigger;
}

SHORT Input::GetRightTrigger(int controllerId)
{
    return controllerStates_[controllerId].Gamepad.bRightTrigger;
}

SHORT Input::GetLeftStickX(int controllerId)
{
    return controllerStates_[controllerId].Gamepad.sThumbLX;
}

SHORT Input::GetLeftStickY(int controllerId)
{
    return controllerStates_[controllerId].Gamepad.sThumbLY;
}

SHORT Input::GetRightStickX(int controllerId)
{
    return controllerStates_[controllerId].Gamepad.sThumbRX;
}

SHORT Input::GetRightStickY(int controllerId)
{

    return controllerStates_[controllerId].Gamepad.sThumbRY;
}

bool Input::IsLeftOnStick(int controllerId, StickType stickType)
{
    if (stickType == LeftStick)
    {
        return GetLeftStickX(controllerId) < -STICK_THRESHOLD;
    }
    else if (stickType == RightStick) 
    {
        return GetRightStickX(controllerId) < -STICK_THRESHOLD;
    }
    return false;
}

bool Input::IsRightOnStick(int controllerId, StickType stickType) 
{
    if (stickType == LeftStick) 
    {
        return GetLeftStickX(controllerId) > STICK_THRESHOLD;
    }
    else if (stickType == RightStick)
    {
        return GetRightStickX(controllerId) > STICK_THRESHOLD;
    }
    return false;
}

bool Input::IsUpOnStick(int controllerId, StickType stickType)
{
    if (stickType == LeftStick)
    {
        return GetLeftStickY(controllerId) > STICK_THRESHOLD;
    }
    else if (stickType == RightStick)
    {
        return GetRightStickY(controllerId) > STICK_THRESHOLD;
    }
    return false;
}


bool Input::IsDownOnStick(int controllerId, StickType stickType)
{
    if (stickType == LeftStick)
    {
        return GetLeftStickY(controllerId) < -STICK_THRESHOLD;
    }
    else if (stickType == RightStick) 
    {
        return GetRightStickY(controllerId) < -STICK_THRESHOLD;
    }
    return false;
}

bool Input::IsTriggerOnStick(int controllerId, StickType stickType)
{
    if (stickType == LeftStick)
    {
        return (GetLeftStickX(controllerId) > STICK_THRESHOLD ||
            GetLeftStickX(controllerId) < -STICK_THRESHOLD ||
            GetLeftStickY(controllerId) > STICK_THRESHOLD ||
            GetLeftStickY(controllerId) < -STICK_THRESHOLD);
    }
    else if (stickType == RightStick)
    {
        return (GetRightStickX(controllerId) > STICK_THRESHOLD ||
            GetRightStickX(controllerId) < -STICK_THRESHOLD ||
            GetRightStickY(controllerId) > STICK_THRESHOLD ||
            GetRightStickY(controllerId) < -STICK_THRESHOLD);
    }
    return false;
}

void Input::VibrateController(int controllerId, float leftMotorSpeed, float rightMotorSpeed)
{
    if (controllerId < 0 || controllerId >= 4) return;

    // 値を0.0f～1.0f にクランプ
    leftMotorSpeed = MyMax<float>(0.0f, MyMin<float>(1.0f, leftMotorSpeed));
    rightMotorSpeed = MyMax<float>(0.0f, MyMin<float>(1.0f, rightMotorSpeed));

    XINPUT_VIBRATION vibration = { 0 };
    vibration.wLeftMotorSpeed = static_cast<WORD>(leftMotorSpeed * 65535.0f);
    vibration.wRightMotorSpeed = static_cast<WORD>(rightMotorSpeed * 65535.0f);

    XInputSetState(controllerId, &vibration);
}

void Input::StartVibration(int controllerId, float leftMotorSpeed, float rightMotorSpeed, float durationSeconds)
{
    if (controllerId < 0 || controllerId >= 4) return;

    leftMotorSpeed = MyMax<float>(0.0f, MyMin<float>(1.0f, leftMotorSpeed));
    rightMotorSpeed = MyMax<float>(0.0f, MyMin<float>(1.0f, rightMotorSpeed));


    // 振動を開始
    VibrateController(controllerId, leftMotorSpeed, rightMotorSpeed);

    // タイマーをセット
    vibrationTimers[controllerId] = durationSeconds;
}