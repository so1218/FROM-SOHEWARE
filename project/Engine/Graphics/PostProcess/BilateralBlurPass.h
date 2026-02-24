#pragma once
#include "IPostEffect.h"
#include "PSOManager.h"

class BilateralBlurPass : public IPostEffect
{
public:
    void Initialize(Engine* engine, UINT width, UINT height, PSOManager* psoManager);

    // IPostEffectの純粋仮想関数（使わないので空実装）
    void Execute(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_DESCRIPTOR_HANDLE inputSRV) override {}

    // Blur専用のExecute（SSAO結果、法線、深度を受け取る）
    void Execute(
        ID3D12GraphicsCommandList* cmdList,
        D3D12_GPU_DESCRIPTOR_HANDLE ssaoSRV,
        D3D12_GPU_DESCRIPTOR_HANDLE normalSRV,
        D3D12_GPU_DESCRIPTOR_HANDLE depthSRV
    );

    BilateralBlurSettings* GetSettings() { return settingsData_; }

    // 中間バッファのSRVを解放するためのデストラクタ
    ~BilateralBlurPass() override;

private:
    PSOManager* psoManager_ = nullptr;

    // 横パス用と縦パス用の定数バッファ
    Microsoft::WRL::ComPtr<ID3D12Resource> cbBlurX_;
    Microsoft::WRL::ComPtr<ID3D12Resource> cbBlurY_;
    BilateralBlurSettings* blurXData_ = nullptr;
    BilateralBlurSettings* blurYData_ = nullptr;

    BilateralBlurSettings settingsDataTemp_;
    BilateralBlurSettings* settingsData_ = &settingsDataTemp_;

    // --- 中間バッファ（横ブラー結果を一時保存するため） ---
    // ※IPostEffectの基底が持つ変数とは別にもう1セット持ちます
    Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource_;
    D3D12_CPU_DESCRIPTOR_HANDLE intermediateRTV_;
    uint32_t intermediateSRVIndex_ = 0;
};