#include "pch.h"
#include "Input.h"
#include "TimeManager.h"

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "Xinput.lib")

namespace FS
{

namespace
{

constexpr float kMaxStickValue = 32767.0f;
// 最後に触った機器を決めるときに、スティックを触ったとみなす傾き
constexpr SHORT kStickTouchThreshold = 0x4000;

bool IsDown(BYTE state) { return (state & 0x80) != 0; }

}

Input& Input::GetInstance()
{
    static Input instance;
    return instance;
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

    HRESULT hr = DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&directInput_, nullptr);
    assert(SUCCEEDED(hr));

    hr = directInput_->CreateDevice(GUID_SysKeyboard, &keyboard_, NULL);
    assert(SUCCEEDED(hr));
    hr = keyboard_->SetDataFormat(&c_dfDIKeyboard);
    assert(SUCCEEDED(hr));
    hr = keyboard_->SetCooperativeLevel(hwnd_, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
    assert(SUCCEEDED(hr));

    hr = directInput_->CreateDevice(GUID_SysMouse, &mouse_, NULL);
    assert(SUCCEEDED(hr));
    hr = mouse_->SetDataFormat(&c_dfDIMouse);
    assert(SUCCEEDED(hr));
    hr = mouse_->SetCooperativeLevel(hwnd_, DISCL_NONEXCLUSIVE | DISCL_BACKGROUND);
    assert(SUCCEEDED(hr));

    // 起動時にパッドがつながっていれば、パッドで遊ぶものとして始める
    UpdatePads();
    if (isPadConnected_[0])
    {
        lastUsedDevice_ = Device::Pad;
    }
}

void Input::Update()
{
    memcpy(prevKeys_, keys_, sizeof(keys_));
    prevMouseState_ = mouseState_;

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

    GetCursorPos(&mousePosition_);
    ScreenToClient(hwnd_, &mousePosition_);

    UpdatePads();
    UpdateLastUsedDevice();
}

void Input::Finalize()
{
    for (int i = 0; i < kMaxPadCount; ++i)
    {
        SetVibration(i, 0.0f, 0.0f);
    }
}

void Input::UpdatePads()
{
    const float deltaTime = TimeManager::GetInstance()->GetUnscaledDeltaTime();

    for (int i = 0; i < kMaxPadCount; ++i)
    {
        prevPadStates_[i] = padStates_[i];
        isPadConnected_[i] = (XInputGetState(i, &padStates_[i]) == ERROR_SUCCESS);

        if (vibrationTimers_[i] > 0.0f)
        {
            vibrationTimers_[i] -= deltaTime;
            if (vibrationTimers_[i] <= 0.0f)
            {
                vibrationTimers_[i] = 0.0f;
                SetVibration(i, 0.0f, 0.0f);
            }
        }
    }
}

void Input::UpdateLastUsedDevice()
{
    // 遊ぶのは 0 番のパッドだけ
    const XINPUT_GAMEPAD& pad = padStates_[0].Gamepad;
    const bool isPadTouched = isPadConnected_[0] && (
        pad.wButtons != 0 ||
        pad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD || pad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD ||
        std::abs(pad.sThumbLX) > kStickTouchThreshold || std::abs(pad.sThumbLY) > kStickTouchThreshold ||
        std::abs(pad.sThumbRX) > kStickTouchThreshold || std::abs(pad.sThumbRY) > kStickTouchThreshold);
    if (isPadTouched)
    {
        lastUsedDevice_ = Device::Pad;
        return;
    }

    // マウスの移動は机に当たっただけでも起きるので、キーとボタンだけで判定する
    if (std::any_of(std::begin(keys_), std::end(keys_), IsDown) ||
        std::any_of(std::begin(mouseState_.rgbButtons), std::end(mouseState_.rgbButtons), IsDown))
    {
        lastUsedDevice_ = Device::KeyboardMouse;
    }
}

bool Input::IsKeyPressed(BYTE key) const { return IsDown(keys_[key]); }
bool Input::IsKeyTriggered(BYTE key) const { return IsDown(keys_[key]) && !IsDown(prevKeys_[key]); }
bool Input::IsKeyReleased(BYTE key) const { return !IsDown(keys_[key]) && IsDown(prevKeys_[key]); }

bool Input::IsMouseButtonPressed(MouseButton button) const
{
    return IsDown(mouseState_.rgbButtons[static_cast<int>(button)]);
}

bool Input::IsMouseButtonTriggered(MouseButton button) const
{
    const int index = static_cast<int>(button);
    return IsDown(mouseState_.rgbButtons[index]) && !IsDown(prevMouseState_.rgbButtons[index]);
}

bool Input::IsMouseButtonReleased(MouseButton button) const
{
    const int index = static_cast<int>(button);
    return !IsDown(mouseState_.rgbButtons[index]) && IsDown(prevMouseState_.rgbButtons[index]);
}

Vector2 Input::GetMousePosition() const
{
    return { static_cast<float>(mousePosition_.x), static_cast<float>(mousePosition_.y) };
}

Vector2 Input::GetMouseDelta() const
{
    return { static_cast<float>(mouseState_.lX), static_cast<float>(mouseState_.lY) };
}

float Input::GetMouseWheel() const
{
    return static_cast<float>(mouseState_.lZ);
}

bool Input::IsPadConnected(int padIndex) const
{
    return IsValidPad(padIndex) && isPadConnected_[padIndex];
}

bool Input::IsPadButtonDown(const XINPUT_GAMEPAD& pad, PadButton button)
{
    switch (button)
    {
    case PadButton::LT: return pad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
    case PadButton::RT: return pad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
    default:
    {
        const WORD mask = static_cast<WORD>(button);
        return (pad.wButtons & mask) == mask;
    }
    }
}

bool Input::IsPadButtonPressed(PadButton button, int padIndex) const
{
    return IsPadConnected(padIndex) && IsPadButtonDown(padStates_[padIndex].Gamepad, button);
}

bool Input::IsPadButtonTriggered(PadButton button, int padIndex) const
{
    return IsPadConnected(padIndex) &&
        IsPadButtonDown(padStates_[padIndex].Gamepad, button) && !IsPadButtonDown(prevPadStates_[padIndex].Gamepad, button);
}

bool Input::IsPadButtonReleased(PadButton button, int padIndex) const
{
    return IsPadConnected(padIndex) &&
        !IsPadButtonDown(padStates_[padIndex].Gamepad, button) && IsPadButtonDown(prevPadStates_[padIndex].Gamepad, button);
}

Vector2 Input::GetStick(Stick stick, float deadzone, int padIndex) const
{
    if (!IsPadConnected(padIndex)) return { 0.0f, 0.0f };

    // 端まで倒しても 1 に届かないパッドがあるので、少し手前で 1 にする
    constexpr float kOuterEdge = 0.95f;

    const XINPUT_GAMEPAD& pad = padStates_[padIndex].Gamepad;
    const bool isLeft = (stick == Stick::Left);
    const float x = std::clamp((isLeft ? pad.sThumbLX : pad.sThumbRX) / kMaxStickValue, -1.0f, 1.0f);
    const float y = std::clamp((isLeft ? pad.sThumbLY : pad.sThumbRY) / kMaxStickValue, -1.0f, 1.0f);

    const float length = std::sqrt(x * x + y * y);
    if (length <= deadzone) return { 0.0f, 0.0f };

    // 向きは保ったまま、デッドゾーンの外側を 0 ~ 1 に広げ直す
    const float strength = std::clamp((length - deadzone) / (kOuterEdge - deadzone), 0.0f, 1.0f);
    return { x / length * strength, y / length * strength };
}

void Input::StartVibration(float leftMotorSpeed, float rightMotorSpeed, float durationSeconds, int padIndex)
{
    if (!IsValidPad(padIndex)) return;

    SetVibration(padIndex, leftMotorSpeed, rightMotorSpeed);
    vibrationTimers_[padIndex] = durationSeconds;
}

void Input::SetVibration(int padIndex, float leftMotorSpeed, float rightMotorSpeed)
{
    XINPUT_VIBRATION vibration = {};
    vibration.wLeftMotorSpeed = static_cast<WORD>(std::clamp(leftMotorSpeed, 0.0f, 1.0f) * 65535.0f);
    vibration.wRightMotorSpeed = static_cast<WORD>(std::clamp(rightMotorSpeed, 0.0f, 1.0f) * 65535.0f);
    XInputSetState(padIndex, &vibration);
}

}
