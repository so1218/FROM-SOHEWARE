#include "pch.h"
#include "WorldInteractionSystem.h"
#include "Engine.h"
#include "PropertyBinder.h"
#include "Terrain.h"
#include "WorldInteractionPass.h"

namespace FE
{

    WorldInteractionSystem::WorldInteractionSystem(FE::Engine* engine)
        : engine_(engine)
    {
        binder_ = std::make_unique<FE::PropertyBinder>(engine_, "WorldInteraction");
    }

    void WorldInteractionSystem::Initialize()
    {
        // 初期値の設定
        constants_.worldSize = 50.0f;
        constants_.trailDuration = 0.5f;

        binder_->Bind("TrailDuration", &constants_.trailDuration, 0.5f, 0.05f, 0.01f, 5.0f);
        binder_->Bind("WorldSize", &constants_.worldSize, 50.0f, 1.0f, 10.0f, 200.0f);
    }

    void WorldInteractionSystem::Update()
    {
        // Terrain がセットされていれば、ハイトマップの原点やサイズを動的に同期
        if (terrain_) 
        {
            constants_.terrainCenter = terrain_->GetOriginXZ();
            constants_.terrainSize = terrain_->GetSizeXZ();
            constants_.terrainHeightScale = terrain_->GetHeightScale();
        }
    }

    void WorldInteractionSystem::Draw()
    {
        // 今フレームのパラメータを RendererManager に送信
        engine_->GetRendererManager()->SubmitWorldInteractionParams(constants_);
    }

    void WorldInteractionSystem::DebugDraw()
    {
#ifdef ENABLE_IMGUI
        ImGui::Begin("ワールドインタラクション");

        binder_->Draw("TrailDuration", "足跡の持続時間 (秒)");
        binder_->Draw("WorldSize", "カバーするワールド範囲 (m)");

        ImGui::Separator();
        ImGui::Text("インタラクションマップ プレビュー");

        // RenderPipeline から最新の SRV インデックスを取得
        uint32_t srvIndex = engine_->GetRendererManager()->GetWorldInteractionSRVIndex();

        if (srvIndex != 0)
        {
            // メインヒープから GPU ハンドルを取得
            D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = engine_->GetSRVManager()->GetSRVHandleGPU(srvIndex);
            if (gpuHandle.ptr != 0)
            {
                ImGui::Image(
                    (ImTextureID)gpuHandle.ptr,
                    ImVec2(256, 256)
                );
            }
        }

        ImGui::End();
#endif

    }

}