#pragma once
#include "IPostEffect.h"
#include "PSOManager.h"

class SSRPass : public IPostEffect {
public:
    void Initialize(Engine* engine, UINT width, UINT height, PSOManager* psoManager);

    void Execute(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_DESCRIPTOR_HANDLE inputSRV) override {}

    // SSR専用のExecute
    void Execute(
        ID3D12GraphicsCommandList* cmdList,
        D3D12_CPU_DESCRIPTOR_HANDLE sceneCPU,   
        D3D12_CPU_DESCRIPTOR_HANDLE normalCPU,  
        D3D12_CPU_DESCRIPTOR_HANDLE depthCPU,   
        D3D12_CPU_DESCRIPTOR_HANDLE materialCPU 
    );

    SSRSettings* GetSettings() { return ssaoData_; }

private:
    PSOManager* psoManager_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> cbSSR_;
    SSRSettings* ssaoData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> passHeap_;
};