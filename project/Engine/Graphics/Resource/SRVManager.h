#pragma once
#include "SRVAllocator.h" 

namespace FE
{

class SRVManager
{
public:
    // SRVヒープとアロケータを初期化
    void Initialize(ID3D12Device* device, uint32_t maxDescriptors);

    // リソースとSRV設定からSRVを作成し、ヒープ内のインデックスを返す
    uint32_t CreateSRV(ID3D12Resource* resource, const D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc);

    // 構造化バッファ専用のSRV作成 
    uint32_t CreateStructuredBufferSRV(ID3D12Resource* resource, uint32_t numElements, uint32_t stride);

    // 構造化バッファ専用のUAV作成
    uint32_t CreateStructuredBufferUAV(ID3D12Resource* resource, uint32_t numElements, uint32_t stride);

    // Append / Consumeバッファ用（カウンタ付き）UAV作成
    uint32_t CreateAppendStructuredBufferUAV(ID3D12Resource* resource, ID3D12Resource* counterResource, uint32_t numElements, uint32_t stride);

    // ExecuteIndirect用など、Rawバッファ専用のSRV作成
    uint32_t CreateRawBufferSRV(ID3D12Resource* resource, uint32_t sizeInBytes);

    // ExecuteIndirect用など、Rawバッファ専用のUAV作成
    uint32_t CreateRawBufferUAV(ID3D12Resource* resource, uint32_t sizeInBytes);

    // UAVを作成してインデックスを返す
    uint32_t CreateUAV(ID3D12Resource* resource, const D3D12_UNORDERED_ACCESS_VIEW_DESC& uavDesc);

    // 指定したインデックスのSRVを解放
    void FreeSRV(uint32_t index);

    // インデックスからGPUハンドルを取得
    D3D12_GPU_DESCRIPTOR_HANDLE GetSRVHandleGPU(uint32_t index) const;

    // 可視（表側）SRVヒープのCPUハンドルを取得（デバッグ用）
    D3D12_CPU_DESCRIPTOR_HANDLE GetSRVHandleCPU_Visible(uint32_t index) const;

    // コピー処理用（裏側）SRVヒープのCPUハンドルを取得
    D3D12_CPU_DESCRIPTOR_HANDLE GetSRVHandleCPU_ForCopying(uint32_t index) const;

    // 汎用CPUハンドル取得
    D3D12_CPU_DESCRIPTOR_HANDLE GetSRVHandleCPU(uint32_t index) const;

    // コマンドリストにセットするためのSRVヒープを取得
    ID3D12DescriptorHeap* GetSRVHeap() const { return srvHeap_.Get(); }

    uint32_t Allocate() { return allocator_->Allocate(); }

private:
    ID3D12Device* device_ = nullptr;               
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeap_;    // GPU可視用SRVヒープ
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeapCPU_; // CPU操作用SRVヒープ
    uint32_t srvDescriptorSize_ = 0;                     // SRVディスクリプタのサイズ

    std::unique_ptr<SRVAllocator> allocator_;       // SRV割り当て・解放管理
};

}