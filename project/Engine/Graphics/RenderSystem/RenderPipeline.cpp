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
    // RenderCoordinator に深度 SRV インデックスを設定
    renderCoordinator_->SetOffscreenDepthSRVIndex(offscreenDepthSrvIndex);
    
    postEffectManager_ = std::make_unique<PostEffectManager>();
    postEffectManager_->Initialize(
        engine,
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

    // 地形ハイトマップのSRVインデックスを取得
    uint32_t heightMapSRV = rendererManager->GetTerrainHeightMapSRVIndex();

    // ハイトマップが有効な場合のみ WorldInteractionPass を実行
    if (heightMapSRV != 0)
    {
        // WorldInteractionSystem から渡された定数パラメータを Pass に渡す
        worldInteractionPass_->SetConstants(rendererManager->GetWorldInteractionConstants());

        // エンティティリストの転送
        worldInteractionPass_->UpdateEntities(rendererManager->GetInteractionEntities());

        // ワールドインタラクションパスの実行
        Vector2 interactionCenterXZ = rendererManager->GetWorldInteractionCenter();
        worldInteractionPass_->Execute(cmdList, heightMapSRV, interactionCenterXZ);

        // 描画側(Draw3D)に最新のインタラクションテクスチャのデータ・SRVを渡す
        rendererManager->SetWorldInteractionData(
            worldInteractionPass_->GetCurrentSRVIndex(),
            worldInteractionPass_->GetWorldSize(),
            interactionCenterXZ,
            worldInteractionPass_->GetConstantBufferAddress()
        );
    }
    else
    {
        // 地形がないシーンではデフォルト値をセット
        rendererManager->SetWorldInteractionData(0, 0.0f, Vector2(0.0f, 0.0f), 0);
    }

    // シャドウパス
    shadowMap_->TransitionToDepthWrite(cmdList); // ループの前に1回だけバリア

    // バッチの事前準備（1回だけ実行）
    rendererManager->PrepareShadowBatches();

    // カスケードごとの描画
    for (uint32_t i = 0; i < ShadowMap::kNumCascades; ++i)
    {
        shadowMap_->BeginPass(cmdList, i);

        rendererManager->DrawSceneForShadow(i);
    }

    shadowMap_->TransitionToRead(cmdList); // ループの後に1回だけバリア

    // RendererManager に深度リソースのポインタを渡す
    rendererManager->SetOffscreenDepthResource(renderCoordinator_->GetOffscreenDepthResource());
    rendererManager->SetOffscreenColorResource(renderCoordinator_->GetOffscreenColorResource());

    // オフスクリーンパス
    renderCoordinator_->BeginOffscreenRender();

    rendererManager->Draw3D();

    renderCoordinator_->EndOffscreenRender();

    // ポストエフェクトパス
    postEffectManager_->ExecutePostEffects(cmdList);

    // 最終合成・トーンマップパス
    renderCoordinator_->BeginFrame();
    postEffectManager_->BeginFinalComposite(cmdList);
    rendererManager->DrawFullScreenQuadWithOffscreenTexture();

    rendererManager->DrawUI();

    postEffectManager_->EndFinalComposite(cmdList);

    // バックバッファへの転送
    D3D12_CPU_DESCRIPTOR_HANDLE backBufferRTV = engine->GetRTVManager()->GetCurrentBackBufferRTVCPUHandle(engine->GetSwapChain());
    cmdList->OMSetRenderTargets(1, &backBufferRTV, FALSE, nullptr);

#ifdef ENABLE_IMGUI
    if (ImGuiManager::IsGuiVisible())
    {
        // GUI オン時: ImGuiのエディタドッキング画面として描画
        engine->GetDebugGuiManager()->EndSceneView();

        ID3D12DescriptorHeap* heaps[] = { engine->GetSRVManager()->GetSRVHeap() };
        cmdList->SetDescriptorHeaps(_countof(heaps), heaps);
        ImGuiManager::EndFrame(cmdList);
    }
    else
    {
        // GUI オフ時: 全画面にゲーム結果を直接描画 
        cmdList->RSSetViewports(1, &engine->GetRenderContext()->GetViewport());
        cmdList->RSSetScissorRects(1, &engine->GetRenderContext()->GetScissorRect());
        rendererManager->DrawFinalResult(postEffectManager_->GetFinalPassSRVIndex());

        // ImGuiの内部コマンドをクリアするためRenderだけ呼び出す
        ImGui::Render();
    }
#else
    // 製品版
    cmdList->RSSetViewports(1, &engine->GetRenderContext()->GetViewport());
    cmdList->RSSetScissorRects(1, &engine->GetRenderContext()->GetScissorRect());
    rendererManager->DrawFinalResult(postEffectManager_->GetFinalPassSRVIndex());
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