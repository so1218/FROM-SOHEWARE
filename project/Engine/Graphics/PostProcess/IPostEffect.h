#pragma once
#include "PostEffectContext.h"
#include "Structures.h"

namespace FE
{

class Engine;
class OffscreenRTVManager;

// ポストエフェクト基底クラス
class IPostEffect
{
protected:
    // 出力用レンダーターゲット
    Microsoft::WRL::ComPtr<ID3D12Resource> textureResource_;
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle_;
    uint32_t srvIndex_ = 0;
    uint32_t uavIndex_ = 0; // CS用にUAVインデックス保持

    // 内部的なリソース状態管理用
    D3D12_RESOURCE_STATES currentState_ = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

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
        DXGI_FORMAT format = DXGI_FORMAT_R16G16B16A16_FLOAT,
        bool isCompute = false 
    );

    // ポストエフェクト実行
    virtual void Execute(ID3D12GraphicsCommandList* cmdList, const PostEffectContext& context, 
        D3D12_GPU_DESCRIPTOR_HANDLE overrideInput = { 0 }) = 0;

    // 出力SRV取得
    D3D12_GPU_DESCRIPTOR_HANDLE GetSRVHandleGPU();
    uint32_t GetSRVIndex() const { return srvIndex_; }

    // CS用バリア：SRV -> UAV
    void PreCompute(ID3D12GraphicsCommandList* cmdList) {
        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            textureResource_.Get(),
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        cmdList->ResourceBarrier(1, &barrier);
    }

    // CS用バリア：UAV -> SRV
    void PostCompute(ID3D12GraphicsCommandList* cmdList) {
        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            textureResource_.Get(),
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        cmdList->ResourceBarrier(1, &barrier);
    }

protected:
    // 描画開始処理
    void PreDraw(ID3D12GraphicsCommandList* cmdList);

    // 描画終了処理
    void PostDraw(ID3D12GraphicsCommandList* cmdList);
};

}