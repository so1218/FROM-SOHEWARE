#pragma once
#include "IPostEffect.h"
#include "PSOManager.h"

class DownsamplePass : public IPostEffect
{
    PSOManager* psoManager_ = nullptr;
    Engine* engine_ = nullptr;
    // Downsampleは単純な縮小コピーなら定数バッファ不要の場合が多いですが、
    // 必要ならここに設定を追加してください。

    Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer_;
    // ★追加: マッピングしたデータへのポインタ
    BlurSettings* cbData_ = nullptr;
public:
    void Initialize(Engine* engine, UINT w, UINT h, PSOManager* pso);

    void Execute(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_DESCRIPTOR_HANDLE inputSRV) override;

};