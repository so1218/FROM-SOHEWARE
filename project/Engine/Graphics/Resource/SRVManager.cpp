#include "SRVManager.h"
#include "DescriptorHeapManager.h"
#include "Logger.h"

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

void SRVManager::CreateStructuredBufferSRV(uint32_t index, ID3D12Resource* resource, uint32_t numElements, uint32_t stride)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_UNKNOWN; // 構造化バッファの場合UNKNOWN
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements = numElements;      // kMaxInstances
    srvDesc.Buffer.StructureByteStride = stride;   // sizeof
    srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

    // 指定されたインデックスのハンドルを取得
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandleVisible = GetSRVHandleCPU_Visible(index);
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandleCopy = GetSRVHandleCPU_ForCopying(index);

    // SRVを作成（表と裏の両方のヒープに書き込む）
    device_->CreateShaderResourceView(resource, &srvDesc, cpuHandleVisible);
    device_->CreateShaderResourceView(resource, &srvDesc, cpuHandleCopy);
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