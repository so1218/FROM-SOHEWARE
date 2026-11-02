#pragma once

namespace FS
{

class RenderContext
{
public:
	// renderWidth/Height はゲームを描く解像度、outputWidth/Height はスワップチェーンの大きさ
	RenderContext(uint32_t renderWidth, uint32_t renderHeight, uint32_t outputWidth, uint32_t outputHeight);

    // ゲームを描くとき
    const D3D12_VIEWPORT& GetViewport() const { return viewport_; }
    const D3D12_RECT& GetScissorRect() const { return scissorRect_; }

    // 描いた画面をスワップチェーンに出すとき。縦横比を保ち、余った所は黒い帯になる
    const D3D12_VIEWPORT& GetOutputViewport() const { return outputViewport_; }
    const D3D12_RECT& GetOutputScissorRect() const { return outputScissorRect_; }

private:
    D3D12_VIEWPORT viewport_{};
    D3D12_RECT scissorRect_{};
    D3D12_VIEWPORT outputViewport_{};
    D3D12_RECT outputScissorRect_{};
};

}
