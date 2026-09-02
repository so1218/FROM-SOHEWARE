#pragma once
#include "IPostEffect.h"
#include "PSOManager.h"

namespace FE
{

class SSRPass : public IPostEffect
{
public:
    void Initialize(Engine* engine, UINT width, UINT height, PSOManager* psoManager);

    void Execute(ID3D12GraphicsCommandList* cmdList, const PostEffectContext& context,
        D3D12_GPU_DESCRIPTOR_HANDLE overrideInput = { 0 }) override;

    SSRSettings* GetSettings() { return cbData_; }

private:
    PSOManager* psoManager_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer_;
    SSRSettings* cbData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> passHeap_;

    // 出力用の中間テクスチャ
    Microsoft::WRL::ComPtr<ID3D12Resource> outputRes_;
    uint32_t outputUavIndex_;
    uint32_t outputSrvIndex_;
};

}