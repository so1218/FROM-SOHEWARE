#pragma once
#include "Structures.h"
#include <d3d12.h>
#include <wrl.h>
#include <externals/DirectXTex/d3dx12.h>

class Engine;
class SRVManager;
class OffscreenRTVManager;

// ポストエフェクト基底クラス
class IPostEffect
{
protected:
    // 出力用レンダーターゲット
    Microsoft::WRL::ComPtr<ID3D12Resource> textureResource_;
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle_;
    uint32_t srvIndex_ = 0;

    // 描画設定
    D3D12_VIEWPORT viewport_;
    D3D12_RECT scissorRect_;

    Engine* engine_ = nullptr;

public:
    virtual ~IPostEffect();

    // 共通初期化
    void InitializeBase(
        Engine* engine,
        UINT width,
        UINT height,
        DXGI_FORMAT format = DXGI_FORMAT_R16G16B16A16_FLOAT
    );

    // ポストエフェクト実行
    virtual void Execute(
        ID3D12GraphicsCommandList* cmdList,
        D3D12_GPU_DESCRIPTOR_HANDLE inputSRV
    ) = 0;

    // 出力SRV取得
    D3D12_GPU_DESCRIPTOR_HANDLE GetSRVHandleGPU();
    uint32_t GetSRVIndex() const { return srvIndex_; }

protected:
    // 描画開始処理
    void PreDraw(ID3D12GraphicsCommandList* cmdList);

    // 描画終了処理
    void PostDraw(ID3D12GraphicsCommandList* cmdList);
};