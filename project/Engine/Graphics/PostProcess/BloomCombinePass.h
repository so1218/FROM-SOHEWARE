#pragma once
#include "IPostEffect.h"
#include "PSOManager.h"

class BloomCombinePass : public IPostEffect
{
private:
    // --- 定数バッファ ---

    // 1. 合成設定 (Bloom強度など: b0想定)
    Microsoft::WRL::ComPtr<ID3D12Resource> cb_;
    CombineSettings* combineData_ = nullptr;

    // --- 依存オブジェクト ---
    PSOManager* psoManager_ = nullptr;
    SRVManager* srvManager_ = nullptr;

    // --- 専用SRVヒープ (t0:Scene, t1:Blur, t2:Depth) ---
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeap_;
    UINT descriptorSize_ = 0;

public:
    void Initialize(Engine* engine, UINT w, UINT h, PSOManager* pso, SRVManager* srvManager);

    void SetupInputViews(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE sceneCPU, D3D12_CPU_DESCRIPTOR_HANDLE blurCPU, D3D12_CPU_DESCRIPTOR_HANDLE depthCPU);

    void Execute(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_DESCRIPTOR_HANDLE /*unused*/) override;

    // 設定変更用のアクセサ
    CombineSettings* GetSettings() const { return combineData_; }
};