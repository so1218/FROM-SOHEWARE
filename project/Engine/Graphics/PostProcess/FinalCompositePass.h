#pragma once
#include "IPostEffect.h"
#include "PSOManager.h"

namespace FE
{

class FinalCompositePass : public IPostEffect
{
public:
    void Initialize(
        Engine* engine,
        uint32_t w,
        uint32_t h,
        PSOManager* pso,
        SRVManager* srvManager
    );

    // 入力テクスチャ設定
    void SetupInputViews(
        ID3D12Device* device,
        D3D12_CPU_DESCRIPTOR_HANDLE sceneCPU,
        D3D12_CPU_DESCRIPTOR_HANDLE bloomCPU,
        D3D12_CPU_DESCRIPTOR_HANDLE dofCPU,
        D3D12_CPU_DESCRIPTOR_HANDLE depthCPU,
        D3D12_CPU_DESCRIPTOR_HANDLE volumetricFogSRV,
        D3D12_CPU_DESCRIPTOR_HANDLE ssaoSRV
    );

    // 合成パス実行
    void Execute(ID3D12GraphicsCommandList* cmdList, const PostEffectContext& context,
        D3D12_GPU_DESCRIPTOR_HANDLE overrideInput = { 0 }) override;

    // 合成設定取得
    FinalCompositeSettings* GetSettings() const { return cbData_; }

    // このパス専用のディスクリプタヒープ
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> passHeap_;

private:
    // 合成用定数バッファ
    Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer_;
    FinalCompositeSettings* cbData_ = nullptr;

    // 依存オブジェクト
    PSOManager* psoManager_ = nullptr;
    SRVManager* srvManager_ = nullptr;

    // 入力テクスチャ用SRVヒープ
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeap_;
    uint32_t descriptorSize_ = 0;
};

}