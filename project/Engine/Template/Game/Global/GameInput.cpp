#include "pch.h"
#include "GameInput.h"
#include "InputBinding.h"

using namespace FS;

namespace
{

using PadButton = Input::PadButton;

InputBinding Key(BYTE key) { return InputBinding::Key(key); }
InputBinding Mouse(Input::MouseButton button) { return InputBinding::Mouse(button); }
InputBinding Pad(PadButton button) { return InputBinding::Pad(button); }

// 割り当て
const std::vector<InputBinding>& GetBindings(GameAction action)
{
    static const std::unordered_map<GameAction, std::vector<InputBinding>> kBindings =
    {
        { GameAction::Jump, { Key(DIK_SPACE), Pad(PadButton::A) } },

        { GameAction::Pause, { Key(DIK_ESCAPE), Pad(PadButton::Menu) } },
        { GameAction::Confirm, { Key(DIK_SPACE), Key(DIK_RETURN), Pad(PadButton::A) } },
        { GameAction::Cancel, { Key(DIK_BACK), Pad(PadButton::B) } },
    };

    const auto it = kBindings.find(action);
    assert(it != kBindings.end() && "GameAction を足したら kBindings にも割り当てを足す");
    return it->second;
}

// 上下左右に割り当てるキー
struct DirectionKeys
{
    BYTE up;
    BYTE down;
    BYTE left;
    BYTE right;
};

constexpr DirectionKeys kMoveKeys = { DIK_W, DIK_S, DIK_A, DIK_D };
constexpr DirectionKeys kLookKeys = { DIK_UP, DIK_DOWN, DIK_LEFT, DIK_RIGHT };

// デッドゾーン
constexpr float kMoveDeadzone = 0.2f;

Vector2 GetKeyDirection(const DirectionKeys& keys)
{
    const Input& input = Input::GetInstance();
    Vector2 direction = { 0.0f, 0.0f };
    if (input.IsKeyPressed(keys.up))    direction.y += 1.0f;
    if (input.IsKeyPressed(keys.down))  direction.y -= 1.0f;
    if (input.IsKeyPressed(keys.right)) direction.x += 1.0f;
    if (input.IsKeyPressed(keys.left))  direction.x -= 1.0f;
    return direction;
}

}

namespace GameInput
{

bool IsPressed(GameAction action)
{
    const auto& bindings = GetBindings(action);
    return std::any_of(bindings.begin(), bindings.end(), [](const InputBinding& binding) { return binding.IsPressed(); });
}

bool IsTriggered(GameAction action)
{
    const auto& bindings = GetBindings(action);
    return std::any_of(bindings.begin(), bindings.end(), [](const InputBinding& binding) { return binding.IsTriggered(); });
}

bool IsReleased(GameAction action)
{
    // 別の割り当てでまだ押していれば、離したことにしない
    if (IsPressed(action)) return false;

    const auto& bindings = GetBindings(action);
    return std::any_of(bindings.begin(), bindings.end(), [](const InputBinding& binding) { return binding.IsReleased(); });
}

Vector2 GetMoveAxis()
{
    Vector2 move = Input::GetInstance().GetStick(Input::Stick::Left, kMoveDeadzone) + GetKeyDirection(kMoveKeys);

    // キーの斜めやスティックとの同時入力で 1 を超えないようにする
    const float length = std::sqrt(move.x * move.x + move.y * move.y);
    return (length > 1.0f) ? move / length : move;
}

Vector2 GetLookAxis(float stickDeadzone)
{
    const Vector2 look = Input::GetInstance().GetStick(Input::Stick::Right, stickDeadzone) + GetKeyDirection(kLookKeys);
    return { std::clamp(look.x, -1.0f, 1.0f), std::clamp(look.y, -1.0f, 1.0f) };
}

float GetMenuVertical()
{
    const Input& input = Input::GetInstance();
    float value = GetKeyDirection(kMoveKeys).y + GetKeyDirection(kLookKeys).y;
    if (input.IsPadButtonPressed(PadButton::Up))   value += 1.0f;
    if (input.IsPadButtonPressed(PadButton::Down)) value -= 1.0f;
    value += input.GetStick(Input::Stick::Left, 0.0f).y;
    return std::clamp(value, -1.0f, 1.0f);
}

bool IsUsingController()
{
    return Input::GetInstance().GetLastUsedDevice() == Input::Device::Pad;
}

void StartVibration(float leftMotorSpeed, float rightMotorSpeed, float durationSeconds)
{
    Input::GetInstance().StartVibration(leftMotorSpeed, rightMotorSpeed, durationSeconds);
}

}
