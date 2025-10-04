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
    GraphicDevice* graphicDevice,
    ID3D12DescriptorHeap* dsvDescriptorHeap,
    Engine* engine)
{
    // 各マネージャーとリソースのポインタを設定
    this->swapChain_ = swapChainManager;
    rtvManager_ = rtvManager;
    this->offscreenRTVManager_ = offscreenRenderTargetManager;
    this->commandManager_ = commandManager;
    this->dsvDescriptorHeap_ = dsvDescriptorHeap;
    this->renderContext_ = renderContext;
    this->fence_ = fence;
    this->fenceEvent_ = fenceEvent;
    this->graphicDevice_ = graphicDevice;
    this->engine_ = engine;

    // 深度ステンシルテクスチャを作成
    depthStencilResource_ = DSVManager::CreateDepthStencilTextureResource(graphicDevice->GetDevice(), kClientWidth, kClientHeight);

    // 深度ステンシルビュー (DSV) を設定
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    graphicDevice->GetDevice()->CreateDepthStencilView(depthStencilResource_.Get(), &dsvDesc, dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart());

    // オフスクリーンの深度ステンシル作成
    offscreenDepth_ = DSVManager::CreateDepthStencilTextureResource(graphicDevice->GetDevice(), kClientWidth, kClientHeight);
    offscreenDSVHeap_ = DSVManager::CreateDSVHeapAndView(graphicDevice->GetDevice(), offscreenDepth_.Get());
}

void RenderCoordinator::BeginFrame()
{
    // 現在のバックバッファのインデックスを取得
    UINT backBufferIndex = swapChain_->GetSwapChain()->GetCurrentBackBufferIndex();

    // バックバッファをPRESENT状態からRENDER_TARGET状態へ遷移
    
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        rtvManager_->swapChainResources[backBufferIndex].Get(),
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET
    );
    commandManager_->GetCommandList()->ResourceBarrier(1, &barrier);
   
    // DSVハンドルを取得
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();

    // 描画先のRTVとDSVを設定
    commandManager_->GetCommandList()->OMSetRenderTargets(1, &rtvManager_->rtvHandles[backBufferIndex], false, &dsvHandle);

    // レンダーターゲットと深度ステンシルをクリア
    float clearColor[] = { 0.0f,0.0f,0.0f,1.0f };
    commandManager_->GetCommandList()->ClearRenderTargetView(rtvManager_->rtvHandles[backBufferIndex], clearColor, 0, nullptr);
    commandManager_->GetCommandList()->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // ビューポートとシザー矩形を設定
    commandManager_->GetCommandList()->RSSetViewports(1, &renderContext_->GetViewport());
    commandManager_->GetCommandList()->RSSetScissorRects(1, &renderContext_->GetScissorRect());
}

void RenderCoordinator::EndFrame()
{
    // 現在のバックバッファのインデックスを取得
    UINT backBufferIndex = swapChain_->GetSwapChain()->GetCurrentBackBufferIndex();

    // バックバッファをRENDER_TARGET状態からPRESENT状態へ遷移
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        rtvManager_->swapChainResources[backBufferIndex].Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT
    );
    commandManager_->GetCommandList()->ResourceBarrier(1, &barrier);
    
    // コマンドリストをクローズ
    HRESULT hr = commandManager_->GetCommandList()->Close();
    assert(SUCCEEDED(hr));

    // コマンドリストをGPUに実行させる
    ID3D12CommandList* commandLists[] = { commandManager_->GetCommandList() };
    commandManager_->GetCommandQueue()->ExecuteCommandLists(1, commandLists);

    // フェンス値を更新し、GPUにシグナルを送る
    fenceValue_++;
    commandManager_->GetCommandQueue()->Signal(fence_.Get(), fenceValue_);

    // 画面を交換
    swapChain_->Present();

    // GPUが現在のフレームの処理を完了するまで待機
    if (fence_->GetCompletedValue() < fenceValue_)
    {
        fence_->SetEventOnCompletion(fenceValue_, fenceEvent_);
        WaitForSingleObject(fenceEvent_, INFINITE);
    }

    // 次のフレームのためにコマンドアロケーターとコマンドリストをリセット
    hr = commandManager_->GetCommandAllocator()->Reset();
    assert(SUCCEEDED(hr));
    hr = commandManager_->GetCommandList()->Reset(commandManager_->GetCommandAllocator(), nullptr);
    assert(SUCCEEDED(hr));
}

void RenderCoordinator::BeginOffscreenRender()
{
    // リソースバリア：オフスクリーンテクスチャをRENDER_TARGET状態に遷移
    barrier_ = CD3DX12_RESOURCE_BARRIER::Transition(
        offscreenRTVManager_->GetOffscreenTexture().Get(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_RENDER_TARGET
    );

    commandManager_->GetCommandList()->ResourceBarrier(1, &barrier_);

    D3D12_CPU_DESCRIPTOR_HANDLE offscreenRTVHandle =
        offscreenRTVManager_->GetRTVDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();

    // オフスクリーン用DSVハンドル取得
    D3D12_CPU_DESCRIPTOR_HANDLE offscreenDSVHandle =
        offscreenDSVHeap_->GetCPUDescriptorHandleForHeapStart();

    // RTV + DSV をバインド
    commandManager_->GetCommandList()->OMSetRenderTargets(
        1, &offscreenRTVHandle, FALSE, &offscreenDSVHandle);

    // クリア
    float clearColor[] = { offscreenRTVManager_->GetClearColor().x, offscreenRTVManager_->GetClearColor().y,
                           offscreenRTVManager_->GetClearColor().z, offscreenRTVManager_->GetClearColor().w };
    commandManager_->GetCommandList()->ClearRenderTargetView(offscreenRTVHandle, clearColor, 0, nullptr);
    commandManager_->GetCommandList()->ClearDepthStencilView(offscreenDSVHandle,
        D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // ビューポート、シザー設定
    commandManager_->GetCommandList()->RSSetViewports(1, &renderContext_->GetViewport());
    commandManager_->GetCommandList()->RSSetScissorRects(1, &renderContext_->GetScissorRect());
}

void RenderCoordinator::EndOffscreenRender()
{
    // リソースバリア：RENDER_TARGETからPIXEL_SHADER_RESOURCEへ遷移
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        offscreenRTVManager_->GetOffscreenTexture().Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );
    commandManager_->GetCommandList()->ResourceBarrier(1, &barrier);
}

