#include "pch.h"
#include "SRVManager.h"
#include "DescriptorHeapManager.h"
#include "Logger.h"

namespace FE
{

void SRVManager::Initialize(ID3D12Device* device, uint32_t maxDescriptors)
{
    device_ = device;
    srvDescriptorSize_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // Shader Visibleヒープ（GPUから見える）を作成
    D3D12_DESCRIPTOR_HEAP_DESC heapDescVisible = {};
    heapDescVisible.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDescVisible.NumDescriptors = maxDescriptors;
    heapDescVisible.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    HRESULT hr = device_->CreateDescriptorHeap(&heapDescVisible, IID_PPV_ARGS(&srvHeap_));
    assert(SUCCEEDED(hr));

    // CPU専用ヒープ（コピー用）を作成
    D3D12_DESCRIPTOR_HEAP_DESC heapDescCPU = {};
    heapDescCPU.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDescCPU.NumDescriptors = maxDescriptors;
    heapDescCPU.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    hr = device_->CreateDescriptorHeap(&heapDescCPU, IID_PPV_ARGS(&srvHeapCPU_));
    assert(SUCCEEDED(hr));

    // SRVの割り当て管理用アロケータを作成
    allocator_ = std::make_unique<SRVAllocator>(maxDescriptors);
}

uint32_t SRVManager::CreateSRV(ID3D12Resource* resource, const D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc)
{
    // 空きインデックスを取得
    uint32_t index = allocator_->Allocate();

    // 表側（GPU可視）ヒープにSRVを作成
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandleVisible = srvHeap_->GetCPUDescriptorHandleForHeapStart();
    cpuHandleVisible.ptr += (SIZE_T)index * srvDescriptorSize_;
    device_->CreateShaderResourceView(resource, &srvDesc, cpuHandleVisible);

    // 裏側（CPU専用）ヒープにも同じSRVを作成
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandleCopy = srvHeapCPU_->GetCPUDescriptorHandleForHeapStart();
    cpuHandleCopy.ptr += (SIZE_T)index * srvDescriptorSize_;
    device_->CreateShaderResourceView(resource, &srvDesc, cpuHandleCopy);

   /* LOG_INFO("SRV Created at Index: {}", index);*/
    return index;
}

uint32_t SRVManager::CreateStructuredBufferSRV(ID3D12Resource* resource, uint32_t numElements, uint32_t stride)
{
    uint32_t index = allocator_->Allocate();

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements = numElements;
    srvDesc.Buffer.StructureByteStride = stride;
    srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

    // 表と裏の両方に作成！
    device_->CreateShaderResourceView(resource, &srvDesc, GetSRVHandleCPU_Visible(index));
    device_->CreateShaderResourceView(resource, &srvDesc, GetSRVHandleCPU_ForCopying(index));

    return index;
}

// 構造化バッファ専用のUAV作成
uint32_t SRVManager::CreateStructuredBufferUAV(ID3D12Resource* resource, uint32_t numElements, uint32_t stride)
{
    uint32_t index = allocator_->Allocate();

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = DXGI_FORMAT_UNKNOWN;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = 0;
    uavDesc.Buffer.NumElements = numElements;
    uavDesc.Buffer.StructureByteStride = stride;

    // 表と裏の両方に作成！
    device_->CreateUnorderedAccessView(resource, nullptr, &uavDesc, GetSRVHandleCPU_Visible(index));
    device_->CreateUnorderedAccessView(resource, nullptr, &uavDesc, GetSRVHandleCPU_ForCopying(index));

    return index;
}

uint32_t SRVManager::CreateAppendStructuredBufferUAV(ID3D12Resource* resource, ID3D12Resource* counterResource, uint32_t numElements, uint32_t stride)
{
    uint32_t index = allocator_->Allocate();

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = DXGI_FORMAT_UNKNOWN;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = 0;
    uavDesc.Buffer.NumElements = numElements;
    uavDesc.Buffer.StructureByteStride = stride;
    uavDesc.Buffer.CounterOffsetInBytes = 0;
    uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

    // 表と裏の両方に作成
    device_->CreateUnorderedAccessView(resource, counterResource, &uavDesc, GetSRVHandleCPU_Visible(index));
    device_->CreateUnorderedAccessView(resource, counterResource, &uavDesc, GetSRVHandleCPU_ForCopying(index));

    return index;
}

uint32_t SRVManager::CreateRawBufferSRV(ID3D12Resource* resource, uint32_t sizeInBytes)
{
    uint32_t index = allocator_->Allocate();

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;         
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements = sizeInBytes / 4;      
    srvDesc.Buffer.StructureByteStride = 0;           
    srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;  

    // 表と裏の両方に作成
    device_->CreateShaderResourceView(resource, &srvDesc, GetSRVHandleCPU_Visible(index));
    device_->CreateShaderResourceView(resource, &srvDesc, GetSRVHandleCPU_ForCopying(index));

    return index;
}

// ExecuteIndirect用など、Rawバッファ専用のUAV作成
uint32_t SRVManager::CreateRawBufferUAV(ID3D12Resource* resource, uint32_t sizeInBytes)
{
    uint32_t index = allocator_->Allocate();

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.NumElements = sizeInBytes / 4;
    uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;

    // 表と裏の両方に作成！
    device_->CreateUnorderedAccessView(resource, nullptr, &uavDesc, GetSRVHandleCPU_Visible(index));
    device_->CreateUnorderedAccessView(resource, nullptr, &uavDesc, GetSRVHandleCPU_ForCopying(index));

    return index;
}

uint32_t SRVManager::CreateUAV(ID3D12Resource* resource, const D3D12_UNORDERED_ACCESS_VIEW_DESC& uavDesc)
{
    uint32_t index = allocator_->Allocate();

    // 可視ヒープ
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandleVisible = GetSRVHandleCPU_Visible(index);
    device_->CreateUnorderedAccessView(resource, nullptr, &uavDesc, cpuHandleVisible);

    // コピー用ヒープ
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandleCopy = GetSRVHandleCPU_ForCopying(index);
    device_->CreateUnorderedAccessView(resource, nullptr, &uavDesc, cpuHandleCopy);

    return index;
}

void SRVManager::FreeSRV(uint32_t index)
{
    // インデックスをアロケータに返却
    allocator_->Free(index);
   /* LOG_INFO("SRV Freed at Index: {}", index);*/
}

D3D12_GPU_DESCRIPTOR_HANDLE SRVManager::GetSRVHandleGPU(uint32_t index) const
{
    // インデックスからGPUハンドルを計算して返す
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = srvHeap_->GetGPUDescriptorHandleForHeapStart();
    gpuHandle.ptr += (SIZE_T)index * srvDescriptorSize_;
    return gpuHandle;
}

D3D12_CPU_DESCRIPTOR_HANDLE SRVManager::GetSRVHandleCPU_Visible(uint32_t index) const
{
    // 表側ヒープからCPUハンドルを取得（ImGuiなどのデバッグ用）
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = srvHeap_->GetCPUDescriptorHandleForHeapStart();
    cpuHandle.ptr += (SIZE_T)index * srvDescriptorSize_;
    return cpuHandle;
}

D3D12_CPU_DESCRIPTOR_HANDLE SRVManager::GetSRVHandleCPU_ForCopying(uint32_t index) const
{
    // 裏側ヒープからCPUハンドルを取得（コピー元用）
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = srvHeapCPU_->GetCPUDescriptorHandleForHeapStart();
    cpuHandle.ptr += (SIZE_T)index * srvDescriptorSize_;
    return cpuHandle;
}

D3D12_CPU_DESCRIPTOR_HANDLE SRVManager::GetSRVHandleCPU(uint32_t index) const
{
    // 汎用CPUハンドルを取得
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = srvHeap_->GetCPUDescriptorHandleForHeapStart();
    cpuHandle.ptr += (SIZE_T)index * srvDescriptorSize_;
    return cpuHandle;
}

}