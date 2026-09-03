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
#include "WorldInteractionPass.h"

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

    // ワールドインタラクションパスの初期化
    worldInteractionPass_ = std::make_unique<WorldInteractionPass>();
    worldInteractionPass_->Initialize(engine, engine->GetPSOManager());
}

void RenderPipeline::Render(Engine* engine, RendererManager* rendererManager, CommandManager* commandManager, const RenderCameraState& cameraState)
{
    auto* cmdList = commandManager->GetCommandList();

    auto* lightManager = engine->GetLightManager();

    // ライトの方向を取得 
    Vector3 lightDir = lightManager->GetDirectionalLightData()[0].direction;

    // カスケード行列の計算・更新
    lightManager->UpdateCascadedShadows(
        lightDir,
        cameraState.view,       
        cameraState.projection, 
        cameraState.nearClip,        
        cameraState.farClip          
    );

    // カリング用フラスタムを更新
    rendererManager->UpdateCullingFrustums();

    // WorldInteractionSystem から渡された定数パラメータを Pass に渡す
    worldInteractionPass_->SetConstants(rendererManager->GetWorldInteractionConstants());

    // エンティティリストの転送
    worldInteractionPass_->UpdateEntities(rendererManager->GetInteractionEntities());

    // ワールドインタラクションパスの実行 (Draw3Dより前に実行してSRVを更新)
    Vector2 interactionCenterXZ = rendererManager->GetWorldInteractionCenter();

    worldInteractionPass_->Execute(cmdList, rendererManager->GetTerrainHeightMapSRVIndex(), interactionCenterXZ);

    // 描画側(Draw3D)に最新のインタラクションテクスチャのSRVインデックスを渡す
    rendererManager->SetWorldInteractionData(
        worldInteractionPass_->GetCurrentSRVIndex(),
        worldInteractionPass_->GetWorldSize(),
        interactionCenterXZ,
        worldInteractionPass_->GetConstantBufferAddress()
    );

    // シャドウパス
    shadowMap_->TransitionToDepthWrite(cmdList); // ループの前に1回だけバリア

    for (uint32_t i = 0; i < ShadowMap::kNumCascades; ++i)
    {
        shadowMap_->BeginPass(cmdList, i);

        rendererManager->DrawSceneForShadow(i);
    }

    shadowMap_->TransitionToRead(cmdList); // ループの後に1回だけバリア

    // G-Buffer / オフスクリーンパス
    renderCoordinator_->BeginOffscreenRender();

    rendererManager->Draw3D();
    renderCoordinator_->EndOffscreenRender();

    // ポストエフェクトパス
    postEffectManager_->ExecutePostEffects(cmdList);

    // 最終合成・トーンマップパス
    renderCoordinator_->BeginFrame();
    postEffectManager_->BeginFinalComposite(cmdList);
    rendererManager->DrawFullScreenQuadWithOffscreenTexture();

#ifdef ENABLE_IMGUI
    // ImGui有効時ゲーム画面はエディタ内の1ウィンドウとして描画されるため、
    // オフスクリーンテクスチャ合成時でゲーム内UIを乗せる
    rendererManager->DrawUI();
#endif
    postEffectManager_->EndFinalComposite(cmdList);

    // バックバッファへの転送
    D3D12_CPU_DESCRIPTOR_HANDLE backBufferRTV = engine->GetRTVManager()->GetCurrentBackBufferRTVCPUHandle(engine->GetSwapChain());
    cmdList->OMSetRenderTargets(1, &backBufferRTV, FALSE, nullptr);

#ifdef ENABLE_IMGUI
    // ImGuiのSceneViewウィンドウの転送と、ImGui自体の描画終了処理
    engine->GetDebugGuiManager()->EndSceneView();

    ID3D12DescriptorHeap* heaps[] = { engine->GetSRVManager()->GetSRVHeap() };
    cmdList->SetDescriptorHeaps(_countof(heaps), heaps);
    ImGuiManager::EndFrame(cmdList); 

#else
    // 製品版(リリース時)画面全体に描画結果を転送し、
    // その上に直接ゲーム内UIを描画
    cmdList->RSSetViewports(1, &engine->GetRenderContext()->GetViewport());
    cmdList->RSSetScissorRects(1, &engine->GetRenderContext()->GetScissorRect());
    rendererManager->DrawFinalResult(postEffectManager_->GetFinalPassSRVIndex());

    rendererManager->DrawUI(); // 製品版のゲームUI
#endif

    renderCoordinator_->EndFrame();
}

uint32_t RenderPipeline::GetWorldInteractionSRVIndex() const 
{ 
    return worldInteractionPass_->GetCurrentSRVIndex();
}

float RenderPipeline::GetWorldInteractionSize() const 
{ 
    return worldInteractionPass_->GetWorldSize();
}

}