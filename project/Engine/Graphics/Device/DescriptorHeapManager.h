#pragma once

class DescriptorHeapManager
{
public:
    // DescriptorHeapの作成関数
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(
        ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescripters, bool shaderVisible);
    // 特定のインデックスのDescriptorHandleを取得する関数
    D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index);
    D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index);
};