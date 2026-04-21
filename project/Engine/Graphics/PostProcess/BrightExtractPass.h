#pragma once
#include "IPostEffect.h"
#include "PSOManager.h"

namespace FE
{

class BrightExtractPass : public IPostEffect
{
    Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer_;
    BrightExtractSettings* cbData_ = nullptr;
    PSOManager* psoManager_ = nullptr;

public:
    void Initialize(Engine* engine, UINT w, UINT h, PSOManager* pso);

    void Execute(ID3D12GraphicsCommandList* cmdList, const PostEffectContext& context,
        D3D12_GPU_DESCRIPTOR_HANDLE overrideInput = { 0 }) override;

    BrightExtractSettings* GetSettings() { return cbData_; }
};

}