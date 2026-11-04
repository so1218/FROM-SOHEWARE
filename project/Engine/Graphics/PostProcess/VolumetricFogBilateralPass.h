#pragma once
#include "IPostEffect.h"
#include "PSOManager.h"

namespace FS
{

class VolumetricFogBilateralPass : public IPostEffect
{
public:
    void Initialize(Engine* engine, uint32_t width, uint32_t height, PSOManager* psoManager);

    void Execute(ID3D12GraphicsCommandList* cmdList, const PostEffectContext& context,
        D3D12_GPU_DESCRIPTOR_HANDLE overrideInput = { 0 }) override;

    FogBilateralSettings* GetSettings() { return cbData_; }

    // ぼかす前の霧
    void SetRawFogSrvIndex(uint32_t srvIndex) { rawFogSrvIndex_ = srvIndex; }

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> passHeap_;
    FogBilateralSettings* cbData_ = nullptr;
    PSOManager* psoManager_ = nullptr;
    uint32_t rawFogSrvIndex_ = 0;
};

}