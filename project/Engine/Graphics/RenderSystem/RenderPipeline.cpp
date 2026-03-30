#include "pch.h"
#include "RenderPipeline.h"
#include "Engine.h" 
#include "ShadowMap.h"
#include "PostEffectManager.h"
#include "RenderCoordinator.h"
#include "RendererManager.h"
#include "CommandManager.h"
#include "ImGuiManager.h"
#include "SRVManager.h"
#include "DSVManager.h"

namespace FE
{

RenderPipeline::RenderPipeline() = default;
RenderPipeline::~RenderPipeline() = default;

void RenderPipeline::Initialize(Engine* engine,
    D3D12_CPU_DESCRIPTOR_HANDLE mainDsvHandle,
    D3D12_CPU_DESCRIPTOR_HANDLE offscreenDsvHandle)
{
    // RenderCoordinatorの初期化
    renderCoordinator_ = std::make_unique<RenderCoordinator>();
    renderCoordinator_->Initialize(
        engine->GetSwapChain(),
        engine->GetRTVManager(),
        engine->GetOffscreenRTVManager(),
        engine->GetCommandManager(),
        engine->GetRenderContext(),
        engine->GetFence(),        
        engine->GetFenceEvent(),    
        engine->GetGraphicsDevice(),
        engine,                     
        mainDsvHandle,
        offscreenDsvHandle,
        engine->GetOffscreenDepthResource()
    );

    // PostEffectManagerの初期化
    uint32_t offscreenDepthSrvIndex = engine->GetDSVManager()->GetDSVTextureSRVIndex(1);
    postEffectManager_ = std::make_unique<PostEffectManager>();
    postEffectManager_->Initialize(
        engine,
        Engine::GetClientWidth(),
        Engine::GetClientHeight(),
        engine->GetRootSignatureManager(),
        engine->GetPSOManager(),
        engine->GetSRVManager(),
        offscreenDepthSrvIndex
    );
    postEffectManager_->SetSceneDepthIndex(offscreenDepthSrvIndex);

    // ShadowMapの初期化
    shadowMap_ = std::make_unique<ShadowMap>();
    shadowMap_->Initialize(
        engine->GetGraphicsDevice()->GetDevice(),
        2048, 2048,
        engine->GetSRVManager()
    );
}

void RenderPipeline::Render(Engine* engine, RendererManager* rendererManager, CommandManager* commandManager, const RenderCameraState& cameraState)
{
    auto* cmdList = commandManager->GetCommandList();

    // シャドウパス
    shadowMap_->BeginPass(cmdList);
    rendererManager->DrawSceneForShadow();
    shadowMap_->EndPass(cmdList);

    // G-Buffer / オフスクリーンパス
    renderCoordinator_->BeginOffscreenRender();
    rendererManager->Draw3D();
    renderCoordinator_->EndOffscreenRender();

    // ポストエフェクトパス
    postEffectManager_->ExecutePostEffects(cmdList, cameraState.view, cameraState.projection, cameraState.eyePos);

    // 最終合成・トーンマップパス
    renderCoordinator_->BeginFrame();
    postEffectManager_->BeginFinalComposite(cmdList);
    rendererManager->DrawFullScreenQuadWithOffscreenTexture();
#ifdef IS_DEVELOPMENT
    rendererManager->DrawUI();
#endif
    postEffectManager_->EndFinalComposite(cmdList);

    // バックバッファへの転送
    D3D12_CPU_DESCRIPTOR_HANDLE backBufferRTV = engine->GetRTVManager()->GetCurrentBackBufferRTVCPUHandle(engine->GetSwapChain());
    cmdList->OMSetRenderTargets(1, &backBufferRTV, FALSE, nullptr);

#ifdef IS_DEVELOPMENT
    engine->GetDebugGuiManager()->EndSceneView();
#else
    cmdList->RSSetViewports(1, &engine->GetRenderContext()->GetViewport());
    cmdList->RSSetScissorRects(1, &engine->GetRenderContext()->GetScissorRect());
    rendererManager->DrawFinalResult(postEffectManager_->GetFinalPassSRVIndex());
    rendererManager->DrawUI();
#endif

    // UIとフレーム終了処理
    ID3D12DescriptorHeap* heaps[] = { engine->GetSRVManager()->GetSRVHeap() };
    cmdList->SetDescriptorHeaps(_countof(heaps), heaps);
    ImGuiManager::EndFrame(cmdList);

    renderCoordinator_->EndFrame();
}

}