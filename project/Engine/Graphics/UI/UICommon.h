#pragma once
#include "Vector2.h"
#include "Engine.h"

namespace FS::UI
{

    // 画面上の位置。screenAnchor (0,0) = 左上, (1,1) = 右下
inline Vector2 ResolveScreenPosition(const Vector2& screenAnchor, const Vector2& offset)
{
    return Vector2(
        screenAnchor.x * static_cast<float>(Engine::GetClientWidth()) + offset.x,
        screenAnchor.y * static_cast<float>(Engine::GetClientHeight()) + offset.y);
}

// UTF-8 を 1 文字ずつに分解する
inline std::u32string DecodeUtf8(std::string_view utf8Text)
{
    std::u32string codepoints;

    size_t byteIndex = 0;
    while (byteIndex < utf8Text.size())
    {
        // 先頭のバイトで、その文字が何バイトか決まる
        const unsigned char leadByte = static_cast<unsigned char>(utf8Text[byteIndex]);
        size_t byteLength = 1;
        char32_t codepoint = leadByte;
        if (leadByte >= 0xF0)
        {
            byteLength = 4;
            codepoint = leadByte & 0x07;
        }
        else if (leadByte >= 0xE0)
        {
            byteLength = 3;
            codepoint = leadByte & 0x0F;
        }
        else if (leadByte >= 0xC0)
        {
            byteLength = 2;
            codepoint = leadByte & 0x1F;
        }

        if (byteIndex + byteLength > utf8Text.size())
        {
            break;
        }

        // 2 バイト目以降は下位 6 ビットずつつなげる
        for (size_t followIndex = 1; followIndex < byteLength; ++followIndex)
        {
            codepoint = (codepoint << 6) | (utf8Text[byteIndex + followIndex] & 0x3F);
        }

        codepoints.push_back(codepoint);
        byteIndex += byteLength;
    }
    return codepoints;
}


}
