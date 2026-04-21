#pragma once
#include "IPostEffect.h"
#include "PSOManager.h"

namespace FE
{

// ダウンサンプル用ポストエフェクト
class DownsamplePass : public IPostEffect
{
    PSOManager* psoManager_ = nullptr;        
    Engine* engine_ = nullptr;                

    Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer_; // 定数バッファ
    BlurSettings* cbData_ = nullptr; // CPU側マッピングデータ

public:
    void Initialize(Engine* engine, UINT w, UINT h, PSOManager* pso);
    void Execute(ID3D12GraphicsCommandList* cmdList, const PostEffectContext& context,
        D3D12_GPU_DESCRIPTOR_HANDLE overrideInput) override;
};

}