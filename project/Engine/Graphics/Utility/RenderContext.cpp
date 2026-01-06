#include "RenderContext.h"

RenderContext::RenderContext(uint32_t width, uint32_t height)
{
    // ビューポート
    // クライアント領域のサイズと一緒にして画面全体に表示
    viewport_.Width = FLOAT(width);
    viewport_.Height = FLOAT(height);
    viewport_.TopLeftX = 0;
    viewport_.TopLeftY = 0;
    viewport_.MinDepth = 0.0f;
    viewport_.MaxDepth = 1.0f;

    // シザー矩形
    // ビューポートと同じ矩形が構成されるようにする
    scissorRect_.left = 0;
    scissorRect_.right = width;
    scissorRect_.top = 0;
    scissorRect_.bottom = height;
}