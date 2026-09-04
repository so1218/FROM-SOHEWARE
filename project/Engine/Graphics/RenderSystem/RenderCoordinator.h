#pragma once

namespace FE
{

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

    // オフスクリーンカラーリソース取得 
    ID3D12Resource* GetOffscreenColorResource() const { return offscreenTexColor_.Get(); }
    // オフスクリーン深度リソース取得
    ID3D12Resource* GetOffscreenDepthResource() const { return offscreenDepthResource_; }

    // オフスクリーンカラーテクスチャの SRV (GPU) ハンドル取得
    D3D12_GPU_DESCRIPTOR_HANDLE GetOffscreenColorSRVGPUHandle() const;
    // オフスクリーン深度テクスチャの SRV (GPU) ハンドル取得
    D3D12_GPU_DESCRIPTOR_HANDLE GetOffscreenDepthSRVGPUHandle() const;
    // 深度 SRV インデックスの設定用（初期化時などに保存する場合）
    void SetOffscreenDepthSRVIndex(uint32_t srvIndex) { offscreenDepthSrvIndex_ = srvIndex; }

    // 屈折用コピーテクスチャの SRV ハンドル取得
    D3D12_GPU_DESCRIPTOR_HANDLE GetOpaqueSceneColorSRVGPUHandle() const;

    // 不透明カラーのコピー処理とバリア切り替え
    void CopyOpaqueSceneColor();
    void TransitionDepthToShaderResource();
    void TransitionDepthToDepthWrite();

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
    // 深度テクスチャの SRV インデックスを保持
    uint32_t offscreenDepthSrvIndex_ = 0;

    Microsoft::WRL::ComPtr<ID3D12Resource> offscreenTexColor_;
    Microsoft::WRL::ComPtr<ID3D12Resource> offscreenTexNormal_;
    Microsoft::WRL::ComPtr<ID3D12Resource> offscreenTexMaterial_;
    Microsoft::WRL::ComPtr<ID3D12Resource> offscreenTexVelocity_;

    D3D12_CPU_DESCRIPTOR_HANDLE offscreenRtvColor_;
    D3D12_CPU_DESCRIPTOR_HANDLE offscreenRtvNormal_;
    D3D12_CPU_DESCRIPTOR_HANDLE offscreenRtvMaterial_;
    D3D12_CPU_DESCRIPTOR_HANDLE offscreenRtvVelocity_;

    // 現在のオフスクリーンのリソースステートを追跡
    D3D12_RESOURCE_STATES currentOffscreenState_ = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

    // 屈折用に不透明描画結果を保持する複製テクスチャ
    Microsoft::WRL::ComPtr<ID3D12Resource> opaqueSceneCopy_;
    uint32_t opaqueSceneCopySrvIndex_ = 0;
};

}