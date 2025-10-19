#include "SwapChain.h"

#include <cassert>

void SwapChain::Initialize(
    HWND hwnd,
    ID3D12CommandQueue* commandQueue,
    uint32_t width,
    uint32_t height,
    uint32_t bufferCount,
    Microsoft::WRL::ComPtr <IDXGIFactory7> dxgiFactory)
{
    // スワップチェーンを生成する
    swapChainDesc_.Width = width;// 画面の幅。ウィンドウのクライアント領域を同じものにしておく
    swapChainDesc_.Height = height;// 画面の高さ。ウィンドウのクライアント領域を同じものにしておく
    swapChainDesc_.Format = DXGI_FORMAT_R8G8B8A8_UNORM;// 色の形式
    swapChainDesc_.SampleDesc.Count = 1;// マルチサンプルしない
    swapChainDesc_.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;// 描画のターゲットとして利用する
    swapChainDesc_.BufferCount = bufferCount;// バッファ数
    swapChainDesc_.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;// モニタに移したら、中身を破棄
    // ウィンドウハンドル、設定を渡して生成する
    HRESULT hr = dxgiFactory->CreateSwapChainForHwnd(
        commandQueue,
        hwnd,
        &swapChainDesc_,
        nullptr,
        nullptr,
        reinterpret_cast<IDXGISwapChain1**>(dxgiSwapChain_.GetAddressOf())
    );
    assert(SUCCEEDED(hr));
}

void SwapChain::Present()
{
    dxgiSwapChain_->Present(1, 0);
}