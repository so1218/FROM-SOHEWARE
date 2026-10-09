#include "pch.h"
#include "DebugGuiManager.h"
#include "Engine.h"
#include "TimeManager.h"
#include "ImGuiManager.h"
#include "LightManager.h"
#include "MaterialManager.h"
#include "TextureLoader.h"
#include "PostEffectManager.h"
#include "DebugCamera.h"
#include "SRVManager.h"
#include "DebugDraw.h"
#include "PropertyBinder.h"

namespace FE
{

DebugGuiManager::DebugGuiManager() = default;
DebugGuiManager::~DebugGuiManager() = default;

void DebugGuiManager::Initialize(Engine* engine, LightManager* lightManager, MaterialManager* materialManager,
    TextureLoader* textureLoader, PostEffectManager* postEffectManager, DebugCamera* debugCamera)
{
    engine_ = engine;
    lightManager_ = lightManager;
    materialManager_ = materialManager;
    textureLoader_ = textureLoader;
    postEffectManager_ = postEffectManager;
    debugCamera_ = debugCamera;

    engineBinder_ = std::make_unique<PropertyBinder>(engine_, "EngineGlobal");
    if (debugCamera_)
    {
        debugCamera_->BindProperties(*engineBinder_);
    }
}

void DebugGuiManager::Update(Camera* targetCamera)
{
#ifdef ENABLE_IMGUI
    // メインのデバッグウィンドウ
    ImGui::Begin("全体のデバッグ情報・設定");

    auto* time = TimeManager::GetInstance();

    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f), "FPS: %.1f (%.2f ms)",
        time->GetAverageFPS(), time->GetDeltaTime() * 1000.0f);

    ImGui::Spacing();

    bool debugCamEnabled = engine_->GetDebugCamera()->IsEnabled();
    if (ImGui::Checkbox("デバッグカメラ", &debugCamEnabled))
    {
        engine_->GetDebugCamera()->SetEnabled(debugCamEnabled);
    }

    ImGui::SameLine();
    bool showDebugDraw = DebugDraw::IsEnabled();
    if (ImGui::Checkbox("デバッグ描画", &showDebugDraw))
    {
        DebugDraw::SetEnabled(showDebugDraw);
    }

    ImGui::SameLine();
    bool isPaused = time->IsPaused();
    if (ImGui::Checkbox("一時停止", &isPaused))
    {
        if (isPaused) time->Pause();
        else time->Resume();
    }

    ImGui::Separator();

    if (ImGui::CollapsingHeader("デバッグカメラ設定"))
    {
        if (debugCamera_)
        {
            debugCamera_->DebugDraw(*engineBinder_, targetCamera);
        }
    }
    if (ImGui::CollapsingHeader("時間 / パフォーマンス"))
    {
        DrawTimeSettings();
    }
    if (ImGui::CollapsingHeader("描画リソース情報"))
    {
        DrawInformationDisplays();
    }

    ImGui::End();

    if (lightManager_)
    {
        lightManager_->DrawSelectedLightGizmo();
    }

#endif
}

#ifdef ENABLE_IMGUI

void DebugGuiManager::DrawTimeSettings()
{
    TimeManager* time = TimeManager::GetInstance();

    ImGui::SeparatorText("時間制御");

    // タイムスケール操作
    float timeScale = time->GetTimeScale();
    if (ImGui::SliderFloat("タイムスケール", &timeScale, 0.0f, 5.0f, "%.2fx"))
    {
        time->SetTimeScale(timeScale);
    }
    ImGui::SameLine();
    if (ImGui::Button("リセット"))
    {
        time->SetTimeScale(1.0f);
    }

    ImGui::Spacing();
    ImGui::SeparatorText("詳細統計");

    ImGui::Text("瞬間 FPS          : %.1f", time->GetFPS());
    ImGui::Text("DeltaTime (Scaled)  : %.2f ms", time->GetDeltaTime() * 1000.0f);
    ImGui::Text("DeltaTime (Unscaled): %.2f ms", time->GetUnscaledDeltaTime() * 1000.0f);
    ImGui::Text("総実行時間 (Total)  : %.2f s", time->GetTotalTime());
}

void DebugGuiManager::DrawInformationDisplays()
{
    // オブジェクト数
    ImGui::Text("Models: %d / %d", engine_->GetRendererManager()->GetModelCount(), engine_->GetRendererManager()->GetMaxModelCount());
    ImGui::Separator();
    ImGui::Text("Sprites: %d / %d", engine_->GetRendererManager()->GetSpriteCount(), engine_->GetRendererManager()->GetMaxSpriteCount());
    ImGui::Separator();
    ImGui::Text("Lines: %d / %d", engine_->GetRendererManager()->GetLineCount(), engine_->GetRendererManager()->GetMaxLineCount());
    ImGui::Separator();
    ImGui::Text("Particles: %d / %d", engine_->GetRendererManager()->GetParticleCount(), engine_->GetRendererManager()->GetMaxParticleCount());
    ImGui::Separator();
    ImGui::Text("Trails: %d / %d", engine_->GetRendererManager()->GetTrailCount(), engine_->GetRendererManager()->GetMaxTrailCount());
}

void DebugGuiManager::BeginSceneView(
    SRVManager* srvManager,
    uint32_t srvIndexToShow)
{
#ifdef ENABLE_IMGUI
    // GUIがオフの時は SceneView ウィンドウを生成しない
    if (!ImGuiManager::IsGuiVisible()) return;

    // GPUハンドル取得
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = srvManager->GetSRVHandleGPU(srvIndexToShow);

    // リセット要求などのロジック
    ImGuiCond cond = ImGuiCond_FirstUseEver;
    if (ImGuiManager::GetSceneResetRequested())
    {
        cond = ImGuiCond_Always;
        ImGuiManager::ClearSceneResetRequested();
    }

    // ウィンドウ設定
    ImGui::SetNextWindowSize(ImVec2(800, 450), cond);
    ImGui::SetNextWindowPos(ImVec2(0, 0), cond);

    // パディングなしで開始
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Scene");
    ImGui::PopStyleVar();

    // ウィンドウの利用可能なサイズを取得
    ImVec2 windowSize = ImGui::GetContentRegionAvail();

    // アスペクト比計算
    float targetAspect = static_cast<float>(Engine::GetClientWidth()) / static_cast<float>(Engine::GetClientHeight());
    float windowAspect = windowSize.x / windowSize.y;

    ImVec2 finalSize = windowSize;
    if (windowAspect > targetAspect)
    {
        finalSize.x = windowSize.y * targetAspect;
    }
    else
    {
        finalSize.y = windowSize.x / targetAspect;
    }

    // 画像描画位置のオフセット計算
    ImVec2 cursorStart = ImGui::GetCursorPos();
    ImVec2 offset;
    offset.x = (windowSize.x - finalSize.x) * 0.5f;
    offset.y = (windowSize.y - finalSize.y) * 0.5f;

    ImGui::SetCursorPos(ImVec2(cursorStart.x + offset.x, cursorStart.y + offset.y));

    ImGui::Image(reinterpret_cast<ImTextureID>(reinterpret_cast<void*>(gpuHandle.ptr)), finalSize);

    // 座標計算
    ImVec2 vMin = ImGui::GetItemRectMin();
    ImVec2 vMax = ImGui::GetItemRectMax();
    bool isHovered = ImGui::IsItemHovered();

    // ImGuizmoのセットアップ 
    ImGuizmo::SetRect(vMin.x, vMin.y, vMax.x - vMin.x, vMax.y - vMin.y);
    ImGuizmo::SetDrawlist();

    // 計算結果をImGuiManagerへ
    ImGuiManager::SetSceneViewRect(
        Vector2(vMin.x, vMin.y),
        Vector2(vMax.x - vMin.x, vMax.y - vMin.y),
        isHovered
    );

#endif
}

void DebugGuiManager::EndSceneView()
{
    // ウィンドウを閉じるだけ
    ImGui::End();
}

#endif

}