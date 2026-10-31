#pragma once
#include "Vector2.h"

namespace FS
{

// 画面・UI のどこを基準にするか
enum class UIAnchor
{
    TopLeft, TopCenter, TopRight,
    MiddleLeft, Center, MiddleRight,
    BottomLeft, BottomCenter, BottomRight,
};

// (0,0) = 左上, (1,1) = 右下
inline Vector2 ToAnchorVector(UIAnchor anchor)
{
    switch (anchor)
    {
    case UIAnchor::TopLeft:      return Vector2(0.0f, 0.0f);
    case UIAnchor::TopCenter:    return Vector2(0.5f, 0.0f);
    case UIAnchor::TopRight:     return Vector2(1.0f, 0.0f);
    case UIAnchor::MiddleLeft:   return Vector2(0.0f, 0.5f);
    case UIAnchor::Center:       return Vector2(0.5f, 0.5f);
    case UIAnchor::MiddleRight:  return Vector2(1.0f, 0.5f);
    case UIAnchor::BottomLeft:   return Vector2(0.0f, 1.0f);
    case UIAnchor::BottomCenter: return Vector2(0.5f, 1.0f);
    case UIAnchor::BottomRight:  return Vector2(1.0f, 1.0f);
    }
    return Vector2(0.0f, 0.0f);
}

}
