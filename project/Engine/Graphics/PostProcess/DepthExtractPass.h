#pragma once
#include "IPostEffect.h"
#include "PSOManager.h"
#include "Camera.h"

class DepthExtractPass : public IPostEffect
{
    // 定数バッファ
    Microsoft::WRL::ComPtr<ID3D12Resource> cbVS_;
    Microsoft::WRL::ComPtr<ID3D12Resource> cbPS_;
    DepthExtractSettingsVS* vsData_ = nullptr;
    DepthExtractSettingsPS* psData_ = nullptr;

    PSOManager* psoManager_ = nullptr;
    RootSignatureManager* rootSigManager_ = nullptr;

public:
    void Initialize(Engine* engine, UINT w, UINT h, PSOManager* pso, RootSignatureManager* rootSig, Camera* camera);

    void UpdateCamera(Camera* camera);

    // execute: inputSRVは「シーンの深度バッファ(DSV)のSRV」
    void Execute(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_DESCRIPTOR_HANDLE depthSRV) override;
};
