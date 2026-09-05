#pragma once
#include "IPostEffect.h"
#include "PSOManager.h"

namespace FE
{

class BokehBlurPass : public IPostEffect
{
public:
    // 初期化
    void Initialize(Engine* engine, uint32_t width, uint32_t height, PSOManager* psoManager);

    // IPostEffect
    void Execute(ID3D12GraphicsCommandList* cmdList, const PostEffectContext& context,
        D3D12_GPU_DESCRIPTOR_HANDLE overrideInput = { 0 }) override;

    DoFSettings* GetSettings() const { return cbData_; }

private:
    Engine* engine_ = nullptr;

    // DoF設定用CB
    Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer_;
    DoFSettings* cbData_ = nullptr;

    PSOManager* psoManager_ = nullptr;
};

}