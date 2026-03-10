#pragma once

class SwapChain;
class RTVManager;
class OffscreenRTVManager;
class CommandManager;
class RenderContext;
class GraphicsDevice;
class Engine;

class RenderCoordinator
{
public:
    // 初期化
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
        D3D12_CPU_DESCRIPTOR_HANDLE offscreenDsvHandle,
        ID3D12Resource* offscreenDepthResource
    );

    // フレーム描画開始・終了
    void BeginFrame();
    void EndFrame();

    // オフスクリーン描画開始・終了
    void BeginOffscreenRender();
    void EndOffscreenRender();

    // フェンス関連ゲッター
    uint64_t GetFenceValue() const { return fenceValue_; }
    ID3D12Fence* GetFence() const { return fence_.Get(); }

    // オフスクリーンRTV/DSVハンドル取得
    D3D12_CPU_DESCRIPTOR_HANDLE GetOffscreenRTVHandle() const { return offscreenRtvColor_; }
    D3D12_CPU_DESCRIPTOR_HANDLE GetOffscreenDSVHandle() const { return offscreenDsvHandle_; }

    // オフスクリーン深度リソース取得
    ID3D12Resource* GetOffscreenDepthResource() const { return offscreenDepthResource_; }

private:
    // 外部依存オブジェクト
    SwapChain* swapChain_ = nullptr;
    RTVManager* rtvManager_ = nullptr;
    OffscreenRTVManager* offscreenRTVManager_ = nullptr;
    CommandManager* commandManager_ = nullptr;
    RenderContext* renderContext_ = nullptr;
    GraphicsDevice* graphicDevice_ = nullptr;
    Engine* engine_ = nullptr;

    // DSVヒープとハンドル
    ID3D12DescriptorHeap* dsvDescriptorHeap_ = nullptr;
    D3D12_CPU_DESCRIPTOR_HANDLE mainDsvHandle_;      // メイン用DSV
    D3D12_CPU_DESCRIPTOR_HANDLE offscreenDsvHandle_; // オフスクリーンDSV

    // フェンス管理
    Microsoft::WRL::ComPtr<ID3D12Fence> fence_ = nullptr;
    HANDLE fenceEvent_ = nullptr;
    uint64_t fenceValue_ = 0;

    // オフスクリーン深度リソース
    ID3D12Resource* offscreenDepthResource_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> offscreenTexColor_;
    Microsoft::WRL::ComPtr<ID3D12Resource> offscreenTexNormal_;
    Microsoft::WRL::ComPtr<ID3D12Resource> offscreenTexMaterial_;

    D3D12_CPU_DESCRIPTOR_HANDLE offscreenRtvColor_;
    D3D12_CPU_DESCRIPTOR_HANDLE offscreenRtvNormal_;
    D3D12_CPU_DESCRIPTOR_HANDLE offscreenRtvMaterial_;
};