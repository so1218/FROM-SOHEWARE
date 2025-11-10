#pragma once

#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <cstdint>
#include <vector>

#include "DescriptorHeapManager.h"
#include "SwapChain.h"
#include "Vector.h"
#include "SRVManager.h"

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

class OffscreenRTVManager
{
public:
    void Initialize(ID3D12Device* device, SRVManager* srvManager, DescriptorHeapManager* descriptorManager, UINT rtvDescriptorCount);

    // オフスクリーンレンダーターゲットを作成し、リソースとRTVハンドルを返す
    std::pair<Microsoft::WRL::ComPtr<ID3D12Resource>, D3D12_CPU_DESCRIPTOR_HANDLE>
        CreateOffscreenRenderTarget(UINT width, UINT height, Vector4 clearColor);

    // RTVヒープの取得
    ID3D12DescriptorHeap* GetRTVDescriptorHeap() const { return rtvDescriptorHeap_.Get(); }

    // 現在設定されているクリアカラーを取得
    Vector4 GetClearColor() const { return clearColor_; }

    // SRVインデックスを外部から取得できるようにする
    uint32_t GetOffscreenSRVIndex() const { return offscreenSrvIndex_; }

private:
    ID3D12Device* device_ = nullptr;  

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap_; // RTVヒープ
    UINT rtvDescriptorSize_ = 0;       // 1ディスクリプタのサイズ
    UINT rtvDescriptorCount_ = 0;      // 作成可能なRTV数
    UINT createdRTVCount_ = 0;         // 作成済みRTV数
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHeapStart_; // ヒープ先頭のCPUハンドル

    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> offscreenRTVHandles_;  // オフスクリーンRTVハンドル
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> offscreenTextures_; // オフスクリーンテクスチャ
    Vector4 clearColor_;  // 作成時に設定するクリアカラー
    SRVManager* srvManager_ = nullptr;
    uint32_t offscreenSrvIndex_ = 0;
};