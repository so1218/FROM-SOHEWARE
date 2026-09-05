#pragma once
#include "IPostEffect.h"
#include "PSOManager.h"

namespace FE
{

class SSAOPass : public IPostEffect 
{
public:
    void Initialize(Engine* engine, uint32_t width, uint32_t height, PSOManager* psoManager);

    // IPostEffectの純粋仮想関数
    void Execute(ID3D12GraphicsCommandList* cmdList, const PostEffectContext& context,
        D3D12_GPU_DESCRIPTOR_HANDLE overrideInput = { 0 }) override;

    SSAOSettings* GetSettings() { return cbData_; }

private:
    PSOManager* psoManager_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer_;
    SSAOSettings* cbData_ = nullptr;
};

}