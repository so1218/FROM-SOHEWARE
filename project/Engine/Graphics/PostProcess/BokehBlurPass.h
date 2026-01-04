#pragma once
#include "IPostEffect.h"
#include "PSOManager.h"

class BokehBlurPass : public IPostEffect
{
public:
    // 初期化（定数バッファ作成など）
    void Initialize(Engine* engine, UINT width, UINT height, PSOManager* psoManager);

    // ★追加: 基底クラスの純粋仮想関数を実装（コンパイルエラー回避用）
    void Execute(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_DESCRIPTOR_HANDLE inputSRV) override;

    // 実行（シーンと深度を受け取る）
    // ディスクリプタヒープ上の「シーンSRV」と「深度SRV」のハンドルを受け取る想定
    void Execute(ID3D12GraphicsCommandList* cmdList,
        D3D12_GPU_DESCRIPTOR_HANDLE sceneSrvGPU,
        D3D12_GPU_DESCRIPTOR_HANDLE depthSrvGPU);

private:
    Engine* engine_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer_;
    DoFSettingsData* cbData_ = nullptr;
    PSOManager* psoManager_ = nullptr;
};
