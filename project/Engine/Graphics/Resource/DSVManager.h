#pragma once

namespace FE
{

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
        uint32_t dsvCount
    );

    // 深度ステンシル用リソースとDSVを作成
    // 作成したリソースとCPUハンドルを返す
    D3D12_CPU_DESCRIPTOR_HANDLE CreateDepthStencilView(
        uint32_t width,
        uint32_t height,
        Microsoft::WRL::ComPtr<ID3D12Resource>& outResource
    );

    // DSVヒープを取得
    ID3D12DescriptorHeap* GetHeap() const { return dsvHeap_.Get(); }

    // 指定したDSVに対応するSRVのインデックスを取得
    uint32_t GetDSVTextureSRVIndex(uint32_t dsvIndex) const
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
    uint32_t dsvDescriptorSize_ = 0;
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHeapStart_{};

    // DSVの作成状況管理
    uint32_t createdDSVCount_ = 0;
    uint32_t maxDSVCount_ = 0;

    // 深度ステンシル用テクスチャの保持
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> depthTextures_;

    // 深度テクスチャ用SRVインデックス
    std::vector<uint32_t> depthSrvIndices_;
};

}