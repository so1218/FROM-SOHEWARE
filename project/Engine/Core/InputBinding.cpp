#include "pch.h"
#include "InputBinding.h"

namespace FS
{

bool InputBinding::IsPressed() const
{
    const Input& input = Input::GetInstance();
    switch (source_)
    {
    case Source::Key:   return input.IsKeyPressed(static_cast<BYTE>(code_));
    case Source::Mouse: return input.IsMouseButtonPressed(static_cast<Input::MouseButton>(code_));
    case Source::Pad:   return input.IsPadButtonPressed(static_cast<Input::PadButton>(code_));
    }
    return false;
}

bool InputBinding::IsTriggered() const
{
    const Input& input = Input::GetInstance();
    switch (source_)
    {
    case Source::Key:   return input.IsKeyTriggered(static_cast<BYTE>(code_));
    case Source::Mouse: return input.IsMouseButtonTriggered(static_cast<Input::MouseButton>(code_));
    case Source::Pad:   return input.IsPadButtonTriggered(static_cast<Input::PadButton>(code_));
    }
    return false;
}

bool InputBinding::IsReleased() const
{
    const Input& input = Input::GetInstance();
    switch (source_)
    {
    case Source::Key:   return input.IsKeyReleased(static_cast<BYTE>(code_));
    case Source::Mouse: return input.IsMouseButtonReleased(static_cast<Input::MouseButton>(code_));
    case Source::Pad:   return input.IsPadButtonReleased(static_cast<Input::PadButton>(code_));
    }
    return false;
}

}
