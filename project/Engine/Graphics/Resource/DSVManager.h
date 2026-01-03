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
    // 初期化
    void Initialize(
        ID3D12Device* device,
        DescriptorHeapManager* descriptorManager,
        SRVManager* srvManager,
        UINT dsvCount
    );

    // 深度ステンシル用リソースとDSVを作成
    // 作成したリソースとCPUハンドルを返す
    D3D12_CPU_DESCRIPTOR_HANDLE CreateDepthStencilView(
        UINT width,
        UINT height,
        Microsoft::WRL::ComPtr<ID3D12Resource>& outResource
    );

    // DSVヒープを取得
    ID3D12DescriptorHeap* GetHeap() const { return dsvHeap_.Get(); }

    // 指定したDSVに対応するSRVのインデックスを取得
    uint32_t GetDSVTextureSRVIndex(UINT dsvIndex) const
    {
        if (dsvIndex < depthSrvIndices_.size())
        {
            return depthSrvIndices_[dsvIndex];
        }
        return 0;
    }

private:
    ID3D12Device* device_ = nullptr;
    SRVManager* srvManager_ = nullptr;

    // DSV用ディスクリプタヒープ
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvHeap_;
    UINT dsvDescriptorSize_ = 0;
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHeapStart_{};

    // DSVの作成状況管理
    UINT createdDSVCount_ = 0;
    UINT maxDSVCount_ = 0;

    // 深度ステンシル用テクスチャの保持
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> depthTextures_;

    // 深度テクスチャ用SRVインデックス
    std::vector<uint32_t> depthSrvIndices_;
};