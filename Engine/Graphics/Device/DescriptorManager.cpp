#include "DescriptorManager.h"

#include <string>

// DescriptorManagerクラス内の関数：ディスクリプタヒープを作成
Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DescriptorManager::DescriptorManager::CreateDescriptorHeap(
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

// 指定インデックスのCPUディスクリプタハンドルを取得
D3D12_CPU_DESCRIPTOR_HANDLE DescriptorManager::GetCPUDescriptorHandle(
    ID3D12DescriptorHeap* descriptorHeap,  // 対象のディスクリプタヒープ
    uint32_t descriptorSize,               // 各ディスクリプタのサイズ
    uint32_t index)                        // インデックス（何番目か）
{
    // ヒープの先頭アドレスを取得
    D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart();

    // インデックスに応じてポインタを加算
    handleCPU.ptr += (descriptorSize * index);

    return handleCPU;  // 対象ディスクリプタのCPUハンドルを返す
}

// 指定インデックスのGPUディスクリプタハンドルを取得
D3D12_GPU_DESCRIPTOR_HANDLE DescriptorManager::GetGPUDescriptorHandle(
    ID3D12DescriptorHeap* descriptorHeap,  // 対象のディスクリプタヒープ
    uint32_t descriptorSize,               // 各ディスクリプタのサイズ
    uint32_t index)                        // インデックス（何番目か）
{
    // ヒープの先頭アドレスを取得（GPU用）
    D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = descriptorHeap->GetGPUDescriptorHandleForHeapStart();

    // インデックスに応じてポインタを加算
    handleGPU.ptr += (descriptorSize * index);

    return handleGPU;  // 対象ディスクリプタのGPUハンドルを返す
}

