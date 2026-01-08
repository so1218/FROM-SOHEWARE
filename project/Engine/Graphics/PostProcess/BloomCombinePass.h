#pragma once
#include "IPostEffect.h"
#include "PSOManager.h"

class BloomCombinePass : public IPostEffect
{
public:
    void Initialize(
        Engine* engine,
        UINT w,
        UINT h,
        PSOManager* pso,
        SRVManager* srvManager
    );

    // 入力テクスチャ設定
    void SetupInputViews(
        ID3D12Device* device,
        D3D12_CPU_DESCRIPTOR_HANDLE sceneCPU,
        D3D12_CPU_DESCRIPTOR_HANDLE bloomCPU,
        D3D12_CPU_DESCRIPTOR_HANDLE dofCPU,
        D3D12_CPU_DESCRIPTOR_HANDLE depthCPU
    );

    // 合成パス実行
    void Execute(
        ID3D12GraphicsCommandList* cmdList,
        D3D12_GPU_DESCRIPTOR_HANDLE /* 未使用 */
    ) override;

    // 合成設定取得
    CombineSettings* GetSettings() const { return combineData_; }

    // このパス専用のディスクリプタヒープ
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> passHeap_;

private:
    // 合成用定数バッファ
    Microsoft::WRL::ComPtr<ID3D12Resource> cb_;
    CombineSettings* combineData_ = nullptr;

    // 依存オブジェクト
    PSOManager* psoManager_ = nullptr;
    SRVManager* srvManager_ = nullptr;

    // 入力テクスチャ用SRVヒープ
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeap_;
    UINT descriptorSize_ = 0;
};