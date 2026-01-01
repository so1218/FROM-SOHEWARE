#include "IPostEffect.h"
#include "Engine.h"

IPostEffect::~IPostEffect()
{
    // ここに書けば、すべての派生クラスで自動的に呼ばれます
    if (engine_->srvManager_ && srvIndex_ != 0)
    {
        engine_->srvManager_->FreeSRV(srvIndex_);
        srvIndex_ = 0;
    }
}

void IPostEffect::InitializeBase(Engine* engine, UINT width, UINT height, DXGI_FORMAT format)
{
    // メンバ変数に保存
    engine_ = engine;
    // 1. OffscreenRTVManagerを使ってレンダーターゲットとRTVを作成
        // ※ clearColorは黒固定とします
    Vector4 clearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // CreateOffscreenRenderTarget は (Resource, CPUHandle) のペアを返すと仮定
    auto result = engine->offscreenRTVManager_->CreateOffscreenRenderTarget(width, height, clearColor);

    // 結果をメンバ変数に保存
    textureResource_ = std::get<0>(result); // ComPtr<ID3D12Resource>
    rtvHandle_ = std::get<1>(result); // D3D12_CPU_DESCRIPTOR_HANDLE

    // 2. SRVの設定
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    // 3. SRVを作成してインデックスを保存
    srvIndex_ = engine_->srvManager_->CreateSRV(textureResource_.Get(), srvDesc);

    // 4. ビューポートとシザー矩形の設定（描画時に使うため）
    viewport_ = { 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f };
    scissorRect_ = { 0, 0, static_cast<long>(width), static_cast<long>(height) };
}

// SRVハンドルを取得する関数
D3D12_GPU_DESCRIPTOR_HANDLE IPostEffect::GetSRVHandleGPU()
{
    // srvManager_ を経由してハンドルを返す
    if (engine_->srvManager_) {
        return engine_->srvManager_->GetSRVHandleGPU(srvIndex_);
    }
    return D3D12_GPU_DESCRIPTOR_HANDLE{ 0 };
}

void IPostEffect::PreDraw(ID3D12GraphicsCommandList* cmdList) 
{
    // リソースバリア (SRV -> RT)
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        textureResource_.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
    cmdList->ResourceBarrier(1, &barrier);

    // セットアップ
    cmdList->RSSetViewports(1, &viewport_);
    cmdList->RSSetScissorRects(1, &scissorRect_);
    cmdList->OMSetRenderTargets(1, &rtvHandle_, FALSE, nullptr);

    // クリア
    float clearColor[] = { 0, 0, 0, 1 };
    cmdList->ClearRenderTargetView(rtvHandle_, clearColor, 0, nullptr);
}

void IPostEffect::PostDraw(ID3D12GraphicsCommandList* cmdList)
{
    // リソースバリア (RT -> SRV)
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        textureResource_.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cmdList->ResourceBarrier(1, &barrier);
}