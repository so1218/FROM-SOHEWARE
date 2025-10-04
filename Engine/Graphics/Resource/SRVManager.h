#pragma once

#include <d3d12.h>              
#include <wrl/client.h>         
#include "externals/DirectXTex/DirectXTex.h"          

class SRVManager
{
public:
    // SRV作成メソッド
    D3D12_GPU_DESCRIPTOR_HANDLE Create(
        const Microsoft::WRL::ComPtr <ID3D12Resource>& textureResource,
        const DirectX::TexMetadata& metadata,
        ID3D12DescriptorHeap* descriptorHeap,
        const Microsoft::WRL::ComPtr <ID3D12Device>& device,
        uint32_t descriptorSizeSRV,
        uint32_t index
    );

    D3D12_GPU_DESCRIPTOR_HANDLE CreateTexture2DArraySRV(
        const Microsoft::WRL::ComPtr<ID3D12Resource>& textureResource,
        const DirectX::TexMetadata& metadata,
        ID3D12DescriptorHeap* descriptorHeap,
        const Microsoft::WRL::ComPtr<ID3D12Device>& device,
        uint32_t descriptorSizeSRV,
        uint32_t index
    );
    // ゲッター
    D3D12_CPU_DESCRIPTOR_HANDLE GetSrvHandleCPU() const { return srvHandleCPU_; }
    D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandleGPU() const { return srvHandleGPU_; }

private:
    D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU_{};
    D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU_{};
};

