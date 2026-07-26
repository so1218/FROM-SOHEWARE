#pragma once
#include "IPostEffect.h"
#include "PSOManager.h"

namespace FE
{

class SSRPass : public IPostEffect
{
public:
    void Initialize(Engine* engine, UINT width, UINT height, PSOManager* psoManager);

    void Execute(ID3D12GraphicsCommandList* cmdList, const PostEffectContext& context,
        D3D12_GPU_DESCRIPTOR_HANDLE overrideInput = { 0 }) override;

    SSRSettings* GetSettings() { return ssrData_; }

private:
    PSOManager* psoManager_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> cbSSR_;
    SSRSettings* ssrData_ = nullptr;

    // Hi-Zダウンサンプル用の設定定数バッファ
    Microsoft::WRL::ComPtr<ID3D12Resource> cbHiZSettings_[6];
    void* hiZData_[6] = { nullptr };

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> passHeap_;
    uint32_t frameCounter_ = 0;

    // 中間テクスチャリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> hiZRes_;
    Microsoft::WRL::ComPtr<ID3D12Resource> hitResultRes_;
    Microsoft::WRL::ComPtr<ID3D12Resource> resolveRes_;
    // Ping-Pong用に2つのバッファ
    Microsoft::WRL::ComPtr<ID3D12Resource> spatialRes_[2];
    Microsoft::WRL::ComPtr<ID3D12Resource> temporalRes_[2]; 

    // SRV / UAV インデックス
    uint32_t hiZSrvIndex_;
    std::vector<uint32_t> hiZUavIndices_;
    uint32_t spatialUavIndices_[2];
    uint32_t spatialSrvIndices_[2];
    // Hi-Zダウンサンプルの入力用 (各Mip単体のSRV)
    std::vector<uint32_t> hiZMipSrvIndices_;

    uint32_t hitResultUavIndex_, hitResultSrvIndex_;
    uint32_t resolveUavIndex_, resolveSrvIndex_;
    uint32_t temporalUavIndices_[2], temporalSrvIndices_[2];

    const UINT maxHiZMipLevels_ = 6; // Mip0〜Mip5
};

}