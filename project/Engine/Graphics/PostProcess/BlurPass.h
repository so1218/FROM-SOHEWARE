#pragma once
#include "IPostEffect.h"
#include "PSOManager.h"

class BlurPass : public IPostEffect
{
    Engine* engine_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer_;
    BlurSettings* cbData_ = nullptr;
    PSOManager* psoManager_ = nullptr;
    bool isVertical_ = false; // 縦か横か

public:
    void Initialize(Engine* engine, UINT w, UINT h, PSOManager* pso, bool isVertical);

    void Execute(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_DESCRIPTOR_HANDLE inputSRV) override;

    BlurSettings* GetSettings() { return cbData_; }
};