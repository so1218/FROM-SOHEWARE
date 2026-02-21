#pragma once
#include "IPostEffect.h"
#include "PSOManager.h"

class BokehBlurPass : public IPostEffect
{
public:
    // 初期化
    void Initialize(Engine* engine, UINT width, UINT height, PSOManager* psoManager);

    // IPostEffect（未使用）
    void Execute(ID3D12GraphicsCommandList* cmdList,
        D3D12_GPU_DESCRIPTOR_HANDLE inputSRV) override;

    // BokehBlur実行
    void Execute(ID3D12GraphicsCommandList* cmdList,
        D3D12_GPU_DESCRIPTOR_HANDLE sceneSrvGPU,
        D3D12_GPU_DESCRIPTOR_HANDLE depthSrvGPU);

    DoFSettings* GetSettings() const { return cbData_; }

private:
    Engine* engine_ = nullptr;

    // DoF設定用CB
    Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer_;
    DoFSettings* cbData_ = nullptr;

    PSOManager* psoManager_ = nullptr;
};