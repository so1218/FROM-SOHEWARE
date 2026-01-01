#pragma once
#include "Structures.h"
#include <d3d12.h>
#include <wrl.h>
#include <externals/DirectXTex/d3dx12.h>

// 前方宣言
class Engine;
class SRVManager;
class OffscreenRTVManager;

class IPostEffect
{
protected:
    // 出力用リソース
    Microsoft::WRL::ComPtr<ID3D12Resource> textureResource_;
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle_;
    uint32_t srvIndex_ = 0; // SRVManager上のインデックス

    // 画面設定
    D3D12_VIEWPORT viewport_;
    D3D12_RECT scissorRect_;

    // 依存関係
    Engine* engine_ = nullptr;

public:
    virtual ~IPostEffect();

    // 初期化 (共通部分: レンダーターゲット作成)
    void InitializeBase(Engine* engine, UINT width, UINT height, DXGI_FORMAT format = DXGI_FORMAT_R16G16B16A16_FLOAT);

    // 描画実行 (純粋仮想関数)
    // inputSRV: 入力となるテクスチャのハンドル
    virtual void Execute(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_DESCRIPTOR_HANDLE inputSRV) = 0;

    // 次のパスへ渡すためのSRVハンドルを取得
    D3D12_GPU_DESCRIPTOR_HANDLE GetSRVHandleGPU();
    uint32_t GetSRVIndex() const { return srvIndex_; }

protected:
    // 描画開始処理 (バリア: SRV -> RT, Clear, Viewport)
    void PreDraw(ID3D12GraphicsCommandList* cmdList);

    // 描画終了処理 (バリア: RT -> SRV)
    void PostDraw(ID3D12GraphicsCommandList* cmdList);
};