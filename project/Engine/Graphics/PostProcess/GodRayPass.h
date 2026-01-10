#pragma once
#include "IPostEffect.h"
#include "PSOManager.h"

class GodRayPass : public IPostEffect
{
public:
    void Initialize(Engine* engine, UINT w, UINT h, PSOManager* pso);

    // IPostEffect（未使用）
    void Execute(ID3D12GraphicsCommandList* cmdList,
        D3D12_GPU_DESCRIPTOR_HANDLE inputSRV) override;

    void Execute(ID3D12GraphicsCommandList* cmdList,
        D3D12_CPU_DESCRIPTOR_HANDLE sceneHandleCPU,
        D3D12_CPU_DESCRIPTOR_HANDLE depthHandleCPU,
        const Vector2& lightPosUV);

    GodRaySettings* GetSettings() { return cbData_; }

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> passHeap_;
    GodRaySettings* cbData_ = nullptr;
    PSOManager* psoManager_ = nullptr;
};