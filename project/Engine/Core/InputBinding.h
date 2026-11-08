#pragma once
#include "Input.h"

namespace FS
{

// 操作に割り当てる入力 1 つ
class InputBinding
{
public:
    static InputBinding Key(BYTE key) { return InputBinding(Source::Key, key); }
    static InputBinding Mouse(Input::MouseButton button) { return InputBinding(Source::Mouse, static_cast<uint32_t>(button)); }
    // パッドは 0 番を見る
    static InputBinding Pad(Input::PadButton button) { return InputBinding(Source::Pad, static_cast<uint32_t>(button)); }

    bool IsPressed() const;
    bool IsTriggered() const;
    bool IsReleased() const;

private:
    enum class Source
    {
        Key,
        Mouse,
        Pad,
    };

    InputBinding(Source source, uint32_t code) : source_(source), code_(code) {}

    Source source_;
    uint32_t code_;
};

}
