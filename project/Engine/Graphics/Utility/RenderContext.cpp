#include "pch.h"
#include "RenderContext.h"

namespace FS
{

RenderContext::RenderContext(uint32_t renderWidth, uint32_t renderHeight, uint32_t outputWidth, uint32_t outputHeight)
{
    viewport_.Width = FLOAT(renderWidth);
    viewport_.Height = FLOAT(renderHeight);
    viewport_.TopLeftX = 0;
    viewport_.TopLeftY = 0;
    viewport_.MinDepth = 0.0f;
    viewport_.MaxDepth = 1.0f;

    scissorRect_.left = 0;
    scissorRect_.right = renderWidth;
    scissorRect_.top = 0;
    scissorRect_.bottom = renderHeight;

    // 縦横比を保ったまま、出力先に収まる一番大きい大きさで真ん中に置く
    const float scale = std::min(FLOAT(outputWidth) / FLOAT(renderWidth), FLOAT(outputHeight) / FLOAT(renderHeight));
    outputViewport_.Width = FLOAT(renderWidth) * scale;
    outputViewport_.Height = FLOAT(renderHeight) * scale;
    outputViewport_.TopLeftX = (FLOAT(outputWidth) - outputViewport_.Width) * 0.5f;
    outputViewport_.TopLeftY = (FLOAT(outputHeight) - outputViewport_.Height) * 0.5f;
    outputViewport_.MinDepth = 0.0f;
    outputViewport_.MaxDepth = 1.0f;

    outputScissorRect_.left = 0;
    outputScissorRect_.right = outputWidth;
    outputScissorRect_.top = 0;
    outputScissorRect_.bottom = outputHeight;
}

}
