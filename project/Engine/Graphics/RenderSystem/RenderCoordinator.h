#pragma once

#include "SwapChain.h"
#include "RTVManager.h"
#include "CommandManager.h"
#include "RenderContext.h"
#include "DSVManager.h"
#include "GraphicsDevice.h"

#include <d3d12.h>
#include <dxgi1_6.h>   
#include <wrl.h>  
#include <memory>

class Engine;

class RenderCoordinator
{
public:
    void Initialize(
        SwapChain* swapChainManager,
        RTVManager* rtvManager,
        OffscreenRTVManager* offscreenRenderTargetManager,
        CommandManager* commandManager,
        RenderContext* renderContext,
        ID3D12Fence* fence,
        HANDLE fenceEvent,
        GraphicsDevice* graphicDevice,
        ID3D12DescriptorHeap* dsvDescriptorHeap,
        Engine* engine);

    void BeginFrame();
    void EndFrame();

    void BeginOffscreenRender();
    void EndOffscreenRender();

    // ゲッター
    uint64_t GetFenceValue() const { return fenceValue_; }
    ID3D12Fence* GetFence() const { return fence_.Get(); }

private:
    // バリア情報(内部用）
    D3D12_RESOURCE_BARRIER barrier_{};

    // 各種依存オブジェクト(初期化時に外部から渡される）
    SwapChain* swapChain_ = nullptr;
    RTVManager* rtvManager_ = nullptr;
    OffscreenRTVManager* offscreenRTVManager_ = nullptr;
    CommandManager* commandManager_ = nullptr;
    RenderContext* renderContext_ = nullptr;
    GraphicsDevice* graphicDevice_ = nullptr;
    Engine* engine_ = nullptr;
    DescriptorHeapManager* descriptorManager_ = nullptr;

    // 深度ステンシルビューヒープとリソース
    ID3D12DescriptorHeap* dsvDescriptorHeap_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource_ = nullptr;

    // オフスクリーン用 DSV
    Microsoft::WRL::ComPtr<ID3D12Resource> offscreenDepth_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> offscreenDSVHeap_;

    // フェンス関連
    Microsoft::WRL::ComPtr<ID3D12Fence> fence_ = nullptr;
    HANDLE fenceEvent_ = nullptr;
    uint64_t fenceValue_ = 0;
};
