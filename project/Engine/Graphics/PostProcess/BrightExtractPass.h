#pragma once
#include "IPostEffect.h"
#include "PSOManager.h"

class BrightExtractPass : public IPostEffect
{
    Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer_;
    BrightExtractSettings* cbData_ = nullptr;
    PSOManager* psoManager_ = nullptr;

public:
    void Initialize(Engine* engine, UINT w, UINT h, PSOManager* pso);

    void Execute(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_DESCRIPTOR_HANDLE inputSRV) override;

    // ImGui等から設定をいじれるようにアクセサを用意
    BrightExtractSettings* GetSettings() { return cbData_; }
};