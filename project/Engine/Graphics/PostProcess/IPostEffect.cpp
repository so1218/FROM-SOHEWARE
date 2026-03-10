#include "pch.h"
#include "IPostEffect.h"
#include "Engine.h"

IPostEffect::~IPostEffect()
{
    // SRV解放
    if (engine_->GetSRVManager() && srvIndex_ != 0)
    {
        engine_->GetSRVManager()->FreeSRV(srvIndex_);
        srvIndex_ = 0;
    }
}

void IPostEffect::InitializeBase(Engine* engine, UINT width, UINT height, DXGI_FORMAT format)
{
    engine_ = engine;

    // オフスクリーンRT作成
    Vector4 clearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // tupleから3つの値（Resource, RTV, SRVIndex）を直接受け取る
    auto [resource, rtvHandle, srvIndex] =
        engine_->GetOffscreenRTVManager()->CreateOffscreenRenderTarget(
            width, height, clearColor, format
        );

    // 取得した値をメンバ変数に保存
    textureResource_ = resource;
    rtvHandle_ = rtvHandle;
    srvIndex_ = srvIndex; // CreateOffscreenRenderTargetで作ったSRVをそのまま使う

    // 描画設定
    viewport_ = { 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };
    scissorRect_ = { 0, 0, (LONG)width, (LONG)height };
}

D3D12_GPU_DESCRIPTOR_HANDLE IPostEffect::GetSRVHandleGPU()
{
    // 出力SRV取得
    return engine_->GetSRVManager()
        ? engine_->GetSRVManager()->GetSRVHandleGPU(srvIndex_)
        : D3D12_GPU_DESCRIPTOR_HANDLE{ 0 };
}

void IPostEffect::PreDraw(ID3D12GraphicsCommandList* cmdList)
{
    // SRV → RT
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        textureResource_.Get(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_RENDER_TARGET);
    cmdList->ResourceBarrier(1, &barrier);

    // 描画先設定
    cmdList->RSSetViewports(1, &viewport_);
    cmdList->RSSetScissorRects(1, &scissorRect_);
    cmdList->OMSetRenderTargets(1, &rtvHandle_, FALSE, nullptr);

    // クリア
    const float clearColor[] = { 0, 0, 0, 1 };
    cmdList->ClearRenderTargetView(rtvHandle_, clearColor, 0, nullptr);
}

void IPostEffect::PostDraw(ID3D12GraphicsCommandList* cmdList)
{
    // RT → SRV
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        textureResource_.Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cmdList->ResourceBarrier(1, &barrier);
}