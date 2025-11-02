#include "RenderCoordinator.h"
#include "Engine.h"
#include <cassert>

void RenderCoordinator::Initialize(
    SwapChain* swapChainManager,
    RTVManager* rtvManager,
    OffscreenRTVManager* offscreenRenderTargetManager,
    CommandManager* commandManager,
    RenderContext* renderContext,
    ID3D12Fence* fence,
    HANDLE fenceEvent,
    GraphicsDevice* graphicDevice,
    Engine* engine,
    D3D12_CPU_DESCRIPTOR_HANDLE mainDsvHandle,
    D3D12_CPU_DESCRIPTOR_HANDLE offscreenRtvHandle,
    ID3D12Resource* offscreenTexture,
    D3D12_CPU_DESCRIPTOR_HANDLE offscreenDsvHandle)
{
    // 各マネージャーとリソースのポインタを保持
    swapChain_ = swapChainManager;
    rtvManager_ = rtvManager;
    offscreenRTVManager_ = offscreenRenderTargetManager;
    commandManager_ = commandManager;
    renderContext_ = renderContext;
    fence_ = fence;
    fenceEvent_ = fenceEvent;
    graphicDevice_ = graphicDevice;
    engine_ = engine;

    // レンダーターゲット・DSV・オフスクリーン関連のハンドルを保持
    mainDsvHandle_ = mainDsvHandle;
    offscreenRtvHandle_ = offscreenRtvHandle;
    offscreenTexture_ = offscreenTexture;
    offscreenDsvHandle_ = offscreenDsvHandle;
}

void RenderCoordinator::BeginFrame()
{
    // バックバッファを取得し、描画可能状態に遷移
    UINT backBufferIndex = swapChain_->GetSwapChain()->GetCurrentBackBufferIndex();
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        rtvManager_->swapChainResources[backBufferIndex].Get(),
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET
    );
    commandManager_->GetCommandList()->ResourceBarrier(1, &barrier);

    // 描画先のRTVとDSVを設定
    commandManager_->GetCommandList()->OMSetRenderTargets(1, &rtvManager_->rtvHandles[backBufferIndex], false, &mainDsvHandle_);

    // レンダーターゲットと深度ステンシルをクリア
    float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    commandManager_->GetCommandList()->ClearRenderTargetView(rtvManager_->rtvHandles[backBufferIndex], clearColor, 0, nullptr);
    commandManager_->GetCommandList()->ClearDepthStencilView(mainDsvHandle_, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // ビューポートとシザー矩形を設定
    commandManager_->GetCommandList()->RSSetViewports(1, &renderContext_->GetViewport());
    commandManager_->GetCommandList()->RSSetScissorRects(1, &renderContext_->GetScissorRect());
}

void RenderCoordinator::EndFrame()
{
    // バックバッファをプレゼント状態に遷移
    UINT backBufferIndex = swapChain_->GetSwapChain()->GetCurrentBackBufferIndex();
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        rtvManager_->swapChainResources[backBufferIndex].Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT
    );
    commandManager_->GetCommandList()->ResourceBarrier(1, &barrier);

    // コマンドリストをクローズしてGPUに送信
    HRESULT hr = commandManager_->GetCommandList()->Close();
    assert(SUCCEEDED(hr));
    ID3D12CommandList* commandLists[] = { commandManager_->GetCommandList() };
    commandManager_->GetCommandQueue()->ExecuteCommandLists(1, commandLists);

    // フェンスでGPU完了を待つ
    fenceValue_++;
    commandManager_->GetCommandQueue()->Signal(fence_.Get(), fenceValue_);
    swapChain_->Present();

    if (fence_->GetCompletedValue() < fenceValue_)
    {
        fence_->SetEventOnCompletion(fenceValue_, fenceEvent_);
        WaitForSingleObject(fenceEvent_, INFINITE);
    }

    // 次フレーム用にコマンドアロケーターとリストをリセット
    hr = commandManager_->GetCommandAllocator()->Reset();
    assert(SUCCEEDED(hr));
    hr = commandManager_->GetCommandList()->Reset(commandManager_->GetCommandAllocator(), nullptr);
    assert(SUCCEEDED(hr));
}

void RenderCoordinator::BeginOffscreenRender()
{
    // オフスクリーンテクスチャを描画可能状態に遷移
    barrier_ = CD3DX12_RESOURCE_BARRIER::Transition(
        offscreenTexture_,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_RENDER_TARGET
    );
    commandManager_->GetCommandList()->ResourceBarrier(1, &barrier_);

    // オフスクリーンのRTVとDSVを設定
    commandManager_->GetCommandList()->OMSetRenderTargets(1, &offscreenRtvHandle_, FALSE, &offscreenDsvHandle_);

    // クリア
    float clearColor[] = {
        offscreenRTVManager_->GetClearColor().x,
        offscreenRTVManager_->GetClearColor().y,
        offscreenRTVManager_->GetClearColor().z,
        offscreenRTVManager_->GetClearColor().w
    };
    commandManager_->GetCommandList()->ClearRenderTargetView(offscreenRtvHandle_, clearColor, 0, nullptr);
    commandManager_->GetCommandList()->ClearDepthStencilView(offscreenDsvHandle_, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // ビューポートとシザーを設定
    commandManager_->GetCommandList()->RSSetViewports(1, &renderContext_->GetViewport());
    commandManager_->GetCommandList()->RSSetScissorRects(1, &renderContext_->GetScissorRect());
}

void RenderCoordinator::EndOffscreenRender()
{
    // オフスクリーンテクスチャをシェーダーリソース状態に遷移
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        offscreenTexture_,
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );
    commandManager_->GetCommandList()->ResourceBarrier(1, &barrier);
}
