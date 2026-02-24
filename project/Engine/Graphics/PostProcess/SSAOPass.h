#pragma once
#include "IPostEffect.h"
#include "PSOManager.h"

class SSAOPass : public IPostEffect 
{
public:
    void Initialize(Engine* engine, UINT width, UINT height, PSOManager* psoManager);

    // IPostEffectの純粋仮想関数（使わないので空実装）
    void Execute(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_DESCRIPTOR_HANDLE inputSRV) override {}

    // SSAO専用のExecute（法線と深度を受け取る）
    void Execute(
        ID3D12GraphicsCommandList* cmdList,
        D3D12_GPU_DESCRIPTOR_HANDLE normalSRV,
        D3D12_GPU_DESCRIPTOR_HANDLE depthSRV
    );

    SSAOSettings* GetSettings() { return ssaoData_; }

private:
    PSOManager* psoManager_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> cbSSAO_;
    SSAOSettings* ssaoData_ = nullptr;
};