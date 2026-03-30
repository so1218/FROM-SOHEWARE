#include "pch.h"
#include "DescriptorHeapManager.h"

namespace FE
{

// ディスクリプタヒープを作成
Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DescriptorHeapManager::DescriptorHeapManager::CreateDescriptorHeap(
    ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible)
{
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap;

    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = heapType;
    desc.NumDescriptors = numDescriptors;
    desc.Flags = shaderVisible
        ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
        : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    HRESULT hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap));
    assert(SUCCEEDED(hr));

    std::wstring heapName = L"DescriptorHeap_";
    switch (heapType) {
    case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV: heapName += L"CBV_SRV_UAV"; break;
    case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:     heapName += L"SAMPLER";     break;
    case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:         heapName += L"RTV";         break;
    case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:         heapName += L"DSV";         break;
    }
    descriptorHeap->SetName(heapName.c_str());

    return descriptorHeap;
}

// CPUディスクリプタハンドルを取得
D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeapManager::GetCPUDescriptorHandle(
    ID3D12DescriptorHeap* descriptorHeap, // 対象ヒープ
    uint32_t descriptorSize,              // ディスクリプタ1個分のサイズ
    uint32_t index)                       // インデックス
{
    // ヒープ先頭からインデックス分オフセット
    D3D12_CPU_DESCRIPTOR_HANDLE handle = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += descriptorSize * index;
    return handle;
}

// GPUディスクリプタハンドルを取得
D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeapManager::GetGPUDescriptorHandle(
    ID3D12DescriptorHeap* descriptorHeap, // 対象ヒープ
    uint32_t descriptorSize,              // ディスクリプタ1個分のサイズ
    uint32_t index)                       // インデックス
{
    // ヒープ先頭からインデックス分オフセット
    D3D12_GPU_DESCRIPTOR_HANDLE handle = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
    handle.ptr += descriptorSize * index;
    return handle;
}

}