#include "Input.h"

#define STICK_THRESHOLD 0x4000

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "Xinput.lib")

IDirectInput8* Input::directInput_ = nullptr;
IDirectInputDevice8* Input::keyboard_ = nullptr;
BYTE Input::keys_[256] = {};
BYTE Input::preKeys_[256] = {};
IDirectInputDevice8* Input::mouse_ = nullptr;
DIMOUSESTATE Input::mouseState_ = {};
DIMOUSESTATE Input::preMouseState_ = {};
HWND Input::hwnd_ = nullptr;
POINT Input::mousePosition_ = { 0, 0 };

XINPUT_STATE Input::controllerStates_[4] = {};
XINPUT_STATE Input::prevControllerStates_[4] = {};
bool Input::controllerConnected_[4] = {};

Input::Input() {}

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

void Input::Initialize(HINSTANCE hInstance, HWND windowHandle)
{
    HRESULT result;

    // DirectInput
    result = DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8,
        (void**)&directInput_, NULL);
    assert(SUCCEEDED(result));

    // キーボード
    result = directInput_->CreateDevice(GUID_SysKeyboard, &keyboard_, NULL);
    assert(SUCCEEDED(result));

    result = keyboard_->SetDataFormat(&c_dfDIKeyboard);
    assert(SUCCEEDED(result));

    result = keyboard_->SetCooperativeLevel(windowHandle, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
    assert(SUCCEEDED(result));

    // マウス
    result = directInput_->CreateDevice(GUID_SysMouse, &mouse_, NULL);
    assert(SUCCEEDED(result));

    result = mouse_->SetDataFormat(&c_dfDIMouse);
    assert(SUCCEEDED(result));

    result = mouse_->SetCooperativeLevel(windowHandle, DISCL_NONEXCLUSIVE | DISCL_BACKGROUND);
    assert(SUCCEEDED(result));

    hwnd_ = windowHandle;

    // コントローラーも初期化
    for (int i = 0; i < 4; i++) {
        controllerConnected_[i] = false;
        ZeroMemory(&controllerStates_[i], sizeof(XINPUT_STATE));
        ZeroMemory(&prevControllerStates_[i], sizeof(XINPUT_STATE));
    }
}

void Input::Update()
{
    memcpy(preKeys_, keys_, sizeof(keys_));
    memcpy(&preMouseState_, &mouseState_, sizeof(mouseState_));
    memcpy(prevControllerStates_, controllerStates_, sizeof(controllerStates_));
    keyboard_->Acquire();
    mouse_->Acquire();
    keyboard_->GetDeviceState(sizeof(keys_), keys_);
    mouse_->GetDeviceState(sizeof(mouseState_), &mouseState_);
    // マウス座標更新
    POINT pt;
    GetCursorPos(&pt); // 画面座標で取得
    ScreenToClient(hwnd_, &pt); // クライアント座標に変換
    mousePosition_ = pt;
    // コントローラーも取得
    UpdateController();
}

void Input::UpdateController()
{
    // コントローラー
    for (int i = 0; i < 4; i++)
    {
        ZeroMemory(&controllerStates_[i], sizeof(XINPUT_STATE));

        DWORD res = XInputGetState(i, &controllerStates_[i]);

        controllerConnected_[i] = (res == ERROR_SUCCESS);
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
    return keys_[key] && !preKeys_[key];
}
bool Input::IsKeyPressed(BYTE key)
{
    return keys_[key];
}
bool Input::IsKeyReleased(BYTE key)
{
    return !keys_[key] && preKeys_[key];
}
bool Input::IsKeyUp(BYTE key)
{
    return !keys_[key];
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

void Input::VibrateController(int controllerId, WORD leftMotorSpeed, WORD rightMotorSpeed)
{
    if (controllerId < 0 ||
        controllerId >= 4) return;

    XINPUT_VIBRATION vibration = { 0 };
    vibration.wLeftMotorSpeed = leftMotorSpeed;
    vibration.wRightMotorSpeed = rightMotorSpeed;

    XInputSetState(controllerId, &vibration);
}
