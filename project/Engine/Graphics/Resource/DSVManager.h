#pragma once

#include <d3d12.h>             
#include <dxgi1_6.h>           
#include <d3dcommon.h>               
#include <assert.h>      
#include <cstdint>
#include <wrl.h>
#include <vector>

class DescriptorHeapManager;
class SRVManager;

class DSVManager
{
public:
    void Initialize(ID3D12Device* device, DescriptorHeapManager* descriptorManager, SRVManager* srvManager, UINT dsvCount);

    // 深度ステンシルビューの作成
    // リソースを作成し、DSVヒープにDSVを作成
    // 作成したCPUハンドルとリソースを返す
    D3D12_CPU_DESCRIPTOR_HANDLE CreateDepthStencilView(
        UINT width,
        UINT height,
        Microsoft::WRL::ComPtr<ID3D12Resource>& outResource
    );

    // DSVヒープの取得
    ID3D12DescriptorHeap* GetHeap() const { return dsvHeap_.Get(); }

    uint32_t GetDSVTextureSRVIndex(UINT dsvIndex) const 
    {
        if (dsvIndex < depthSrvIndices_.size()) return depthSrvIndices_[dsvIndex];
        return 0; // エラー時は0などを返す
    }

private:
    ID3D12Device* device_ = nullptr;

    // DSVヒープ
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvHeap_;
    UINT dsvDescriptorSize_ = 0;
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHeapStart_{};

    // 作成したDSVの管理用
    UINT createdDSVCount_ = 0; // 作成済みのDSV数
    UINT maxDSVCount_ = 0;     // 最大DSV数

    // DSV用リソースの保持
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> depthTextures_;

    SRVManager* srvManager_ = nullptr;
    std::vector<uint32_t> depthSrvIndices_;
};