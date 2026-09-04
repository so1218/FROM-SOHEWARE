#pragma once
#include "Vector.h"

namespace FE
{

class DescriptorHeapManager;
class SwapChain;
class SRVManager;
class Engine;

class RTVManager
{
public:
    ~RTVManager() { rtvDescriptorHeap_.Reset(); }

    // 初期化：スワップチェーンのバックバッファ用RTVを作成
    void Initialize(ID3D12Device* device, IDXGISwapChain4* swapChain, uint32_t bufferCount, DescriptorHeapManager* descriptorManager);

    uint32_t backBufferCount = 0;                 // バックバッファ数
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};      // RTVの基本設定
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtvHandles;  // 各バックバッファ用RTVハンドル
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap_; // RTVヒープ
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> swapChainResources; // スワップチェーンのバックバッファ
    uint32_t descriptorSizeRTV_;

    // 現在のバックバッファのRTV CPUハンドルを取得
    D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferRTVCPUHandle(SwapChain* swapChainManager);
};

enum class GBufferIndex : uint32_t
{
    Color = 0,
    Normal = 1,
    Material = 2,
    Velocity = 3
};

class OffscreenRTVManager
{
public:
    void Initialize(ID3D12Device* device, SRVManager* srvManager, DescriptorHeapManager* descriptorManager, uint32_t rtvDescriptorCount);

    // オフスクリーンレンダーターゲットを作成し、リソースとRTVハンドルを返す
    std::tuple<Microsoft::WRL::ComPtr<ID3D12Resource>, D3D12_CPU_DESCRIPTOR_HANDLE, uint32_t, uint32_t>
        CreateOffscreenRenderTarget(uint32_t width, uint32_t height, Vector4 clearColor, DXGI_FORMAT format, bool enableUAV = false);

    // RTVヒープの取得
    ID3D12DescriptorHeap* GetRTVDescriptorHeap() const { return rtvDescriptorHeap_.Get(); }

    // 現在設定されているクリアカラーを取得
    Vector4 GetClearColor() const { return clearColor_; }

    // 引数なしの場合は0番目（カラー）を返す
    uint32_t GetOffscreenSRVIndex(uint32_t index) const
    {
        assert(index < offscreenSrvIndices_.size());
        return offscreenSrvIndices_[index];
    }

    // UAVのインデックスを取得する関数
    uint32_t GetOffscreenUAVIndex(uint32_t index) const
    {
        assert(index < offscreenUavIndices_.size());
        return offscreenUavIndices_[index];
    }

private:
    ID3D12Device* device_ = nullptr;  

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap_; // RTVヒープ
    uint32_t rtvDescriptorSize_ = 0;       // 1ディスクリプタのサイズ
    uint32_t rtvDescriptorCount_ = 0;      // 作成可能なRTV数
    uint32_t createdRTVCount_ = 0;         // 作成済みRTV数
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHeapStart_; // ヒープ先頭のCPUハンドル

    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> offscreenRTVHandles_;  // オフスクリーンRTVハンドル
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> offscreenTextures_; // オフスクリーンテクスチャ
    Vector4 clearColor_;  // 作成時に設定するクリアカラー
    SRVManager* srvManager_ = nullptr;

    std::vector<uint32_t> offscreenSrvIndices_;

    // UAVのインデックスを保存する配列
    std::vector<uint32_t> offscreenUavIndices_;
};

}