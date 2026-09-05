#pragma once
#include "IPostEffect.h"
#include "PSOManager.h"

namespace FE
{

class VolumetricFogBilateralPass : public IPostEffect
{
public:
    void Initialize(Engine* engine, uint32_t w, uint32_t h, PSOManager* pso);

    void Execute(ID3D12GraphicsCommandList* cmdList, const PostEffectContext& context,
        D3D12_GPU_DESCRIPTOR_HANDLE overrideInput = { 0 }) override;

    FogBilateralSettings* GetSettings() { return cbData_; }

    // 前パス（生のフォグ）のリソースとSRVインデックスを受け取る
    void SetRawFogInput(ID3D12Resource* resource, uint32_t srvIndex) {
        rawFogResource_ = resource;
        rawFogSrvIndex_ = srvIndex;
    }

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> passHeap_;
    FogBilateralSettings* cbData_ = nullptr;
    PSOManager* psoManager_ = nullptr;

    // 前パスの情報保持用
    ID3D12Resource* rawFogResource_ = nullptr;
    uint32_t rawFogSrvIndex_ = 0;
};

}