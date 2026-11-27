#pragma once
#include "Text.h"

// 文字の見た目をそろえるための共通の設定
namespace UITextStyle
{

// 丸ゴシックの文字に付ける影
inline void ApplyMaruShadow(FS::Text& text)
{
    text.SetShadow({ 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.8627451f }, 11.7f, 1.8f);
}

// 数字に付ける影
inline void ApplyNumberShadow(FS::Text& text)
{
    text.SetShadow({ 1.5f, 1.5f }, { 0.0f, 0.0f, 0.0f, 0.62352943f }, 1.8f, 0.5f);
}

}
