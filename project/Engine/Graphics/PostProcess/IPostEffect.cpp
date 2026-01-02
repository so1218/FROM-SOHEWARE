#include "IPostEffect.h"
#include "Engine.h"

IPostEffect::~IPostEffect()
{
    // SRV解放
    if (engine_->srvManager_ && srvIndex_ != 0)
    {
        engine_->srvManager_->FreeSRV(srvIndex_);
        srvIndex_ = 0;
    }
}

void IPostEffect::InitializeBase(Engine* engine, UINT width, UINT height, DXGI_FORMAT format)
{
    engine_ = engine;

    // オフスクリーンRT作成
    Vector4 clearColor(0.0f, 0.0f, 0.0f, 1.0f);
    auto result = engine_->offscreenRTVManager_->CreateOffscreenRenderTarget(
        width, height, clearColor);

    textureResource_ = std::get<0>(result);
    rtvHandle_ = std::get<1>(result);

    // SRV作成
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    srvIndex_ = engine_->srvManager_->CreateSRV(textureResource_.Get(), srvDesc);

    // 描画設定
    viewport_ = { 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };
    scissorRect_ = { 0, 0, (LONG)width, (LONG)height };
}

D3D12_GPU_DESCRIPTOR_HANDLE IPostEffect::GetSRVHandleGPU()
{
    // 出力SRV取得
    return engine_->srvManager_
        ? engine_->srvManager_->GetSRVHandleGPU(srvIndex_)
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