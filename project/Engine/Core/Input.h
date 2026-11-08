#pragma once
#include "Vector2.h"

namespace FS
{

class Input
{
public:
    static Input& GetInstance();

    Input(const Input&) = delete;
    Input& operator=(const Input&) = delete;

    enum class MouseButton
    {
        Left,
        Right,
        Middle,
    };

    // Xbox コントローラーのボタン
    enum class PadButton : uint32_t
    {
        Up = XINPUT_GAMEPAD_DPAD_UP,
        Down = XINPUT_GAMEPAD_DPAD_DOWN,
        Left = XINPUT_GAMEPAD_DPAD_LEFT,
        Right = XINPUT_GAMEPAD_DPAD_RIGHT,
        Menu = XINPUT_GAMEPAD_START,
        View = XINPUT_GAMEPAD_BACK,
        L3 = XINPUT_GAMEPAD_LEFT_THUMB,
        R3 = XINPUT_GAMEPAD_RIGHT_THUMB,
        LB = XINPUT_GAMEPAD_LEFT_SHOULDER,
        RB = XINPUT_GAMEPAD_RIGHT_SHOULDER,
        A = XINPUT_GAMEPAD_A,
        B = XINPUT_GAMEPAD_B,
        X = XINPUT_GAMEPAD_X,
        Y = XINPUT_GAMEPAD_Y,
        LT = 0x10000,
        RT = 0x20000,
    };

    enum class Stick
    {
        Left,
        Right,
    };

    // 最後に操作された機器
    enum class Device
    {
        KeyboardMouse,
        Pad,
    };

    void Initialize(HINSTANCE hInstance, HWND hwnd);
    void Update();
    void Finalize();

    // キーボード
    bool IsKeyPressed(BYTE key) const;
    bool IsKeyTriggered(BYTE key) const;
    bool IsKeyReleased(BYTE key) const;

    // マウス
    bool IsMouseButtonPressed(MouseButton button) const;
    bool IsMouseButtonTriggered(MouseButton button) const;
    bool IsMouseButtonReleased(MouseButton button) const;
    // ウィンドウの左上からのカーソルの位置 (ピクセル)
    Vector2 GetMousePosition() const;
    // このフレームでマウスが動いた量
    Vector2 GetMouseDelta() const;
    // ホイールを回した量。奥に回すと正で、1 目盛りで 120
    float GetMouseWheel() const;

    // パッド
    bool IsPadConnected(int padIndex = 0) const;
    bool IsPadButtonPressed(PadButton button, int padIndex = 0) const;
    bool IsPadButtonTriggered(PadButton button, int padIndex = 0) const;
    bool IsPadButtonReleased(PadButton button, int padIndex = 0) const;
    // スティックの傾き。各軸 -1 ~ 1 で、倒した量が deadzone 以下なら 0
    Vector2 GetStick(Stick stick, float deadzone, int padIndex = 0) const;
    // durationSeconds 秒たつと止まる。強さは 0 ~ 1
    void StartVibration(float leftMotorSpeed, float rightMotorSpeed, float durationSeconds, int padIndex = 0);

    Device GetLastUsedDevice() const { return lastUsedDevice_; }

private:
    static constexpr int kMaxPadCount = 4;

    Input() = default;
    ~Input();

    void UpdatePads();
    void UpdateLastUsedDevice();
    void SetVibration(int padIndex, float leftMotorSpeed, float rightMotorSpeed);
    static bool IsPadButtonDown(const XINPUT_GAMEPAD& pad, PadButton button);
    static bool IsValidPad(int padIndex) { return padIndex >= 0 && padIndex < kMaxPadCount; }

    IDirectInput8* directInput_ = nullptr;
    HWND hwnd_ = nullptr;

    IDirectInputDevice8* keyboard_ = nullptr;
    BYTE keys_[256] = {};
    BYTE prevKeys_[256] = {};

    IDirectInputDevice8* mouse_ = nullptr;
    DIMOUSESTATE mouseState_ = {};
    DIMOUSESTATE prevMouseState_ = {};
    POINT mousePosition_ = {};

    XINPUT_STATE padStates_[kMaxPadCount] = {};
    XINPUT_STATE prevPadStates_[kMaxPadCount] = {};
    bool isPadConnected_[kMaxPadCount] = {};
    float vibrationTimers_[kMaxPadCount] = {};

    Device lastUsedDevice_ = Device::KeyboardMouse;
};

}
