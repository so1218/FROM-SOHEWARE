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
        Engine* engine,
        D3D12_CPU_DESCRIPTOR_HANDLE mainDsvHandle,
        D3D12_CPU_DESCRIPTOR_HANDLE offscreenRtvHandle, // オフスクリーン用RTV
        ID3D12Resource* offscreenTexture,               // バリア用
        D3D12_CPU_DESCRIPTOR_HANDLE offscreenDsvHandle);

    // フレーム描画開始・終了
    void BeginFrame();
    void EndFrame();

    // オフスクリーン描画開始・終了
    void BeginOffscreenRender();
    void EndOffscreenRender();

    // フェンス関連ゲッター
    uint64_t GetFenceValue() const { return fenceValue_; }
    ID3D12Fence* GetFence() const { return fence_.Get(); }

    // オフスクリーンRTV/DSVのハンドル
    D3D12_CPU_DESCRIPTOR_HANDLE GetOffscreenRTVHandle() const { return offscreenRtvHandle_; }
    D3D12_CPU_DESCRIPTOR_HANDLE GetOffscreenDSVHandle() const { return offscreenDsvHandle_; }

private:
    // リソースバリア用
    D3D12_RESOURCE_BARRIER barrier_{};

    // 外部から渡される依存オブジェクト
    SwapChain* swapChain_ = nullptr;
    RTVManager* rtvManager_ = nullptr;
    OffscreenRTVManager* offscreenRTVManager_ = nullptr;
    CommandManager* commandManager_ = nullptr;
    RenderContext* renderContext_ = nullptr;
    GraphicsDevice* graphicDevice_ = nullptr;
    Engine* engine_ = nullptr;

    // 深度ステンシルヒープとハンドル
    ID3D12DescriptorHeap* dsvDescriptorHeap_ = nullptr;
    D3D12_CPU_DESCRIPTOR_HANDLE mainDsvHandle_;      // メインレンダーターゲット用
    D3D12_CPU_DESCRIPTOR_HANDLE offscreenRtvHandle_; // オフスクリーンRTV
    ID3D12Resource* offscreenTexture_;               // バリア用リソース
    D3D12_CPU_DESCRIPTOR_HANDLE offscreenDsvHandle_; // オフスクリーンDSV

    // フェンス関連
    Microsoft::WRL::ComPtr<ID3D12Fence> fence_ = nullptr;
    HANDLE fenceEvent_ = nullptr;
    uint64_t fenceValue_ = 0;
};