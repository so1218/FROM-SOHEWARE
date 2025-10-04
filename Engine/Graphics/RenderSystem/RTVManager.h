#pragma once

#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <cstdint>
#include <vector>

#include "DescriptorManager.h"
#include "SwapChain.h"
#include "Vector.h"

class Engine;

class RTVManager
{
public:
	~RTVManager() { rtvDescriptorHeap_.Reset(); }

	void Initialize(ID3D12Device* device, IDXGISwapChain4* swapChain, uint32_t bufferCount, uint32_t descriptorSizeRTV, DescriptorManager* descriptorManager);
	uint32_t backBufferCount = 0;
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtvHandles;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap_;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> swapChainResources;
	// 現在のバックバッファのRTV CPUハンドルを返すメソッド
	D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferRTVCPUHandle(SwapChain* swapChainManager);
};

class OffscreenRTVManager
{
public:
    // オフスクリーン用のRTV用ヒープとSRV用ヒープを初期化
    void Initialize(ID3D12Device* device, DescriptorManager* descriptorManager, UINT rtvDescriptorCount);

    // 指定された解像度でオフスクリーンレンダーターゲットを作成し、インデックスを返す
    uint32_t CreateOffscreenRenderTarget(UINT width, UINT height, Vector4 clearColor = Vector4(0.0f, 0.0f, 0.0f, 1.0f));

    uint32_t CreateDepthTexture(UINT width, UINT height);

    // RTVデスクリプタヒープの取得
    ID3D12DescriptorHeap* GetRTVDescriptorHeap() const { return rtvDescriptorHeap_.Get(); }

    // SRV用デスクリプタハンドルを登録し、インデックスを返す
    uint32_t AllocateAndRegisterSRV(
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle,
        D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle);

    // 指定インデックスのRTVハンドルを返す(CPU用）
    D3D12_CPU_DESCRIPTOR_HANDLE GetRTVHandle(UINT index) const
    {
        assert(index < offscreenRTVHandles_.size());
        return offscreenRTVHandles_[index];
    }

    // 指定インデックスのオフスクリーンテクスチャリソースを返す
    Microsoft::WRL::ComPtr<ID3D12Resource> GetOffscreenTexture(size_t index = 0) const {
        if (index < offscreenTextures_.size()) {
            return offscreenTextures_[index];
        }
        return nullptr;
    }

    // 指定インデックスのSRV GPUハンドルを取得
    D3D12_GPU_DESCRIPTOR_HANDLE GetSRVHandleGPU(uint32_t index) const {
        assert(index < srvEntries_.size());
        return srvEntries_[index].gpuHandle;
    }

    // 指定インデックスのSRV CPUハンドルを取得
    D3D12_CPU_DESCRIPTOR_HANDLE GetSRVHandleCPU(uint32_t index) const {
        assert(index < srvEntries_.size());
        return srvEntries_[index].cpuHandle;
    }

    // SRV用のデスクリプタヒープを取得
    ID3D12DescriptorHeap* GetSRVDescriptorHeap() const {
        return srvDescriptorHeap_.Get();
    }

    // SRVエントリの構造体（CPU/GPUハンドルのペア）
    struct SRVEntry {
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
        D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
    };

    D3D12_CPU_DESCRIPTOR_HANDLE GetNextSRVCPUHandle() const {
        D3D12_CPU_DESCRIPTOR_HANDLE handle = srvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
        handle.ptr += srvEntries_.size() * srvDescriptorSize_;
        return handle;
    }

    D3D12_GPU_DESCRIPTOR_HANDLE GetNextSRVGPUHandle() const {
        D3D12_GPU_DESCRIPTOR_HANDLE handle = srvDescriptorHeap_->GetGPUDescriptorHandleForHeapStart();
        handle.ptr += srvEntries_.size() * srvDescriptorSize_;
        return handle;
    }

    Vector4 GetClearColor() const { return clearColor_; }

private:
    ID3D12Device* device_ = nullptr;

    // RTV用のデスクリプタヒープと情報
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap_;
    UINT rtvDescriptorSize_ = 0;
    UINT rtvDescriptorCount_ = 0;
    UINT createdRTVCount_ = 0;
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHeapStart_;  // RTVヒープの先頭ハンドル
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> offscreenRTVHandles_; // 各RTVのハンドル
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> offscreenTextures_; // 各RTVに対応するリソース
    Vector4 clearColor_;

    // SRV用のデスクリプタヒープ（GPUおよびCPU用）
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> cpuOnlySRVDescriptorHeap_;
    UINT srvDescriptorSize_ = 0;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap_;
    UINT dsvDescriptorSize_ = 0;

    std::vector<SRVEntry> srvEntries_; // SRVハンドルのリスト
};