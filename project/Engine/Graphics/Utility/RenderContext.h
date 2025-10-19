#pragma once

#include <d3d12.h>
#include <cstdint>

class RenderContext
{
public:
	RenderContext(uint32_t width, uint32_t height);

    const D3D12_VIEWPORT& GetViewport() const { return viewport_; }
    const D3D12_RECT& GetScissorRect() const { return scissorRect_; }

private:
    D3D12_VIEWPORT viewport_{};
    D3D12_RECT scissorRect_{};
};

