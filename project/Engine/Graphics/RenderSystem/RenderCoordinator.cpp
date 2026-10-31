#include "pch.h"
#include "RenderCoordinator.h"
#include "SwapChain.h"
#include "RTVManager.h"
#include "CommandManager.h"
#include "RenderContext.h"
#include "DSVManager.h"
#include "SRVManager.h"
#include "GraphicsDevice.h"
#include "Engine.h"

namespace FS
{

void RenderCoordinator::Initialize(
    SwapChain* swapChainManager,
    RTVManager* rtvManager,
    OffscreenRTVManager* offscreenRenderTargetManager,
    CommandManager* commandManager,
    RenderContext* renderContext,
    GraphicsDevice* graphicDevice,
    DSVManager* dsvManager, 
    SRVManager* srvManager
)
{
    // 各マネージャーとリソースのポインタを保持
    swapChain_ = swapChainManager;
    rtvManager_ = rtvManager;
    offscreenRTVManager_ = offscreenRenderTargetManager;
    commandManager_ = commandManager;
    renderContext_ = renderContext;
    fence_ = commandManager_->GetFence();
    fenceEvent_ = commandManager_->GetFenceEvent();
    graphicDevice_ = graphicDevice;
    dsvManager_ = dsvManager;
    srvManager_ = srvManager;

    // 深度ステンシルビュー (DSV) の作成
    mainDsvIndex_ = dsvManager->CreateDepthStencilView(Engine::GetClientWidth(), Engine::GetClientHeight());
    offscreenDsvIndex_ = dsvManager->CreateDepthStencilView(Engine::GetClientWidth(), Engine::GetClientHeight());

    // ハンドルとリソースを取得して保持
    mainDsvHandle_ = dsvManager->GetDSVHandle(mainDsvIndex_);
    offscreenDsvHandle_ = dsvManager->GetDSVHandle(offscreenDsvIndex_);
    offscreenDepthResource_ = dsvManager->GetResource(offscreenDsvIndex_);

    // カラー用
    auto [texColor, rtvColor, srvColor, uavIndexColor] = offscreenRTVManager_->CreateOffscreenRenderTarget(
        Engine::GetClientWidth(), Engine::GetClientHeight(), offscreenRTVManager_->GetClearColor(), DXGI_FORMAT_R16G16B16A16_FLOAT);
    offscreenTexColor_ = texColor;
    offscreenRtvColor_ = rtvColor;

    // 法線用
    auto [texNormal, rtvNormal, srvNormal, uavIndexNormal] = offscreenRTVManager_->CreateOffscreenRenderTarget(
        Engine::GetClientWidth(), Engine::GetClientHeight(), Vector4(0, 0, 0, 0), DXGI_FORMAT_R16G16B16A16_FLOAT);
    offscreenTexNormal_ = texNormal;
    offscreenRtvNormal_ = rtvNormal;

    // --------------------------------------------------------
    // 深度リソース用 SRV の作成
    // --------------------------------------------------------
    // SRV ヒープ領域を確保
    offscreenDepthSrvIndex_ = srvManager->Allocate();

    // SRV 設定
    D3D12_SHADER_RESOURCE_VIEW_DESC depthSrvDesc{};
    depthSrvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    depthSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    depthSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    depthSrvDesc.Texture2D.MipLevels = 1;

    // SRV の生成
    graphicDevice_->GetDevice()->CreateShaderResourceView(
        offscreenDepthResource_,
        &depthSrvDesc,
        srvManager->GetSRVHandleCPU(offscreenDepthSrvIndex_)
    );

    // --------------------------------------------------------
    // 屈折用不透明カラーコピーテクスチャの作成
    // --------------------------------------------------------
    CD3DX12_RESOURCE_DESC copyDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        DXGI_FORMAT_R16G16B16A16_FLOAT,
        Engine::GetClientWidth(),
        Engine::GetClientHeight(),
        1,
        1  
    );
    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

    graphicDevice_->GetDevice()->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &copyDesc,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        nullptr,
        IID_PPV_ARGS(&opaqueSceneCopy_)
    );

    opaqueSceneCopySrvIndex_ = srvManager->Allocate();

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 1;

    graphicDevice_->GetDevice()->CreateShaderResourceView(
        opaqueSceneCopy_.Get(),
        &srvDesc,
        srvManager->GetSRVHandleCPU(opaqueSceneCopySrvIndex_)
    );
}

void RenderCoordinator::BeginFrame()
{
    // バックバッファを取得し、描画可能状態に遷移
    uint32_t backBufferIndex = swapChain_->GetSwapChain()->GetCurrentBackBufferIndex();
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
    uint32_t backBufferIndex = swapChain_->GetSwapChain()->GetCurrentBackBufferIndex();
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
    auto* cmdList = commandManager_->GetCommandList();

    D3D12_RESOURCE_BARRIER barriers[2];
    barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(offscreenTexColor_.Get(), currentOffscreenState_, D3D12_RESOURCE_STATE_RENDER_TARGET);
    barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(offscreenTexNormal_.Get(), currentOffscreenState_, D3D12_RESOURCE_STATE_RENDER_TARGET);
    cmdList->ResourceBarrier(2, barriers);

    // 2枚のRTVハンドルを配列にしてセット
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2] = { offscreenRtvColor_, offscreenRtvNormal_ };
    cmdList->OMSetRenderTargets(2, rtvHandles, FALSE, &offscreenDsvHandle_);

    // それぞれをクリアする
    Vector4 cc = offscreenRTVManager_->GetClearColor();
    float clearColorDefault[] = { cc.x, cc.y, cc.z, cc.w };
    float clearColorZero[] = { 0.0f, 0.0f, 0.0f, 0.0f };    // 法線・材質用

    cmdList->ClearRenderTargetView(offscreenRtvColor_, clearColorDefault, 0, nullptr);
    cmdList->ClearRenderTargetView(offscreenRtvNormal_, clearColorZero, 0, nullptr);
    cmdList->ClearDepthStencilView(offscreenDsvHandle_, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // ビューポートとシザーを設定
    cmdList->RSSetViewports(1, &renderContext_->GetViewport());
    cmdList->RSSetScissorRects(1, &renderContext_->GetScissorRect());
}

void RenderCoordinator::EndOffscreenRender()
{
    D3D12_RESOURCE_BARRIER barriers[2];

    // 両方のシェーダーで読めるステートを定義
    D3D12_RESOURCE_STATES readState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

    barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(offscreenTexColor_.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, readState);
    barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(offscreenTexNormal_.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, readState);
    
    commandManager_->GetCommandList()->ResourceBarrier(2, barriers);

    // 次のフレームの BeginOffscreenRender のためにステートを更新
    currentOffscreenState_ = readState;
}

D3D12_GPU_DESCRIPTOR_HANDLE RenderCoordinator::GetOffscreenColorSRVGPUHandle() const
{
    // OffscreenRTVManager から Color の SRV インデックスを取得
    uint32_t colorSrvIndex = offscreenRTVManager_->GetOffscreenSRVIndex(
        static_cast<uint32_t>(GBufferIndex::Color)
    );

    // SRVManager を通して GPU ハンドルに変換して返す
    return srvManager_->GetSRVHandleGPU(colorSrvIndex);
}

D3D12_GPU_DESCRIPTOR_HANDLE RenderCoordinator::GetOffscreenDepthSRVGPUHandle() const
{
    // 保存しておいた深度の SRV インデックスから GPU ハンドルを取得
    return srvManager_->GetSRVHandleGPU(offscreenDepthSrvIndex_);
}

void RenderCoordinator::CopyOpaqueSceneColor()
{
    auto* cmdList = commandManager_->GetCommandList();

    // バリア: offscreenTexColor_ を COPY_SOURCE に、opaqueSceneCopy_ を COPY_DEST に遷移
    D3D12_RESOURCE_BARRIER barriersBefore[2];
    barriersBefore[0] = CD3DX12_RESOURCE_BARRIER::Transition(
        offscreenTexColor_.Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_COPY_SOURCE
    );
    barriersBefore[1] = CD3DX12_RESOURCE_BARRIER::Transition(
        opaqueSceneCopy_.Get(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_COPY_DEST
    );
    cmdList->ResourceBarrier(2, barriersBefore);

    // 高速コピー（GPU内部での転送）
    cmdList->CopyResource(opaqueSceneCopy_.Get(), offscreenTexColor_.Get());

    // バリア: 状態を元に戻す（offscreenTexColor_ は RTV、opaqueSceneCopy_ は SRV）
    D3D12_RESOURCE_BARRIER barriersAfter[2];
    barriersAfter[0] = CD3DX12_RESOURCE_BARRIER::Transition(
        offscreenTexColor_.Get(),
        D3D12_RESOURCE_STATE_COPY_SOURCE,
        D3D12_RESOURCE_STATE_RENDER_TARGET
    );
    barriersAfter[1] = CD3DX12_RESOURCE_BARRIER::Transition(
        opaqueSceneCopy_.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );
    cmdList->ResourceBarrier(2, barriersAfter);
}

void RenderCoordinator::TransitionDepthToShaderResource()
{
    auto* cmdList = commandManager_->GetCommandList();
    D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        offscreenDepthResource_,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );
    cmdList->ResourceBarrier(1, &barrier);
}

void RenderCoordinator::TransitionDepthToDepthWrite()
{
    auto* cmdList = commandManager_->GetCommandList();
    D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        offscreenDepthResource_,
        D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_DEPTH_WRITE
    );
    cmdList->ResourceBarrier(1, &barrier);
}

D3D12_GPU_DESCRIPTOR_HANDLE RenderCoordinator::GetOpaqueSceneColorSRVGPUHandle() const
{
    return srvManager_->GetSRVHandleGPU(opaqueSceneCopySrvIndex_);
}

}
