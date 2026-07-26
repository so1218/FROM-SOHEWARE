#include "pch.h"
#include "LightningSystem.h"
#include "Engine.h" 
#include "PropertyBinder.h" 
#include "TimeManager.h" 

namespace FE
{

LightningSystem::LightningSystem(Engine* engine) : engine_(engine)
{
    binder_ = std::make_unique<PropertyBinder>(engine_, "Lightning");
}

LightningSystem::~LightningSystem()
{
    // アクティブなライトを返却
    auto lightManager = engine_->GetLightManager(); 
    if (lightManager)
    {
        for (const auto& activeLight : activeLights_)
        {
            lightManager->ReturnPointLight(activeLight.lightIndex);
        }
    }
}

void LightningSystem::Initialize()
{
    // カラー設定
    binder_->BindColor("CoreColor", &config_.coreColor, { 1.0f, 1.0f, 1.0f });
    binder_->BindColor("FringeColor", &config_.fringeColor, { 0.1f, 0.5f, 1.0f });
    binder_->BindColor("LightColor", &config_.lightColor, { 0.7f, 0.85f, 1.0f, 1.0f });

    // 発光と形状
    binder_->Bind("Thickness", &config_.thickness, 0.5f, 0.1f, 0.05f, 10.0f);
    binder_->Bind("CoreThickness", &config_.coreThickness, 0.15f, 0.01f, 0.01f, 1.0f);
    binder_->Bind("CorePower", &config_.corePower, 4.0f, 0.1f, 0.1f, 20.0f);
    binder_->Bind("GlowPower", &config_.glowPower, 2.5f, 0.1f, 0.1f, 10.0f);
    binder_->Bind("EmissiveIntensity", &config_.emissiveIntensity, 50.0f, 1.0f, 1.0f, 1000.0f);
    binder_->Bind("FractalDepth", &config_.fractalDepth, 6, 1, 1, 10);
    binder_->Bind("Displacement", &config_.displacement, 20.0f, 1.0f, 0.0f, 100.0f);

    binder_->Bind("BranchProbability", &config_.branchProbability, 0.3f, 0.05f, 0.0f, 1.0f);
    binder_->Bind("BranchLengthScale", &config_.branchLengthScale, 0.6f, 0.05f, 0.1f, 2.0f);

    // 寿命と明滅
    binder_->Bind("DurationMin", &config_.durationMin, 0.08f, 0.01f, 0.01f, 0.5f);
    binder_->Bind("DurationMax", &config_.durationMax, 0.35f, 0.01f, 0.05f, 1.0f);
    binder_->Bind("FlickerSpeed", &config_.flickerSpeed, 60.0f, 1.0f, 0.0f, 200.0f);
    binder_->Bind("FlickerMin", &config_.flickerMin, 0.4f, 0.05f, 0.0f, 2.0f);
    binder_->Bind("FlickerMax", &config_.flickerMax, 1.2f, 0.05f, 0.0f, 5.0f);

    // 落雷ライト
    binder_->Bind("LightIntensityMax", &config_.lightIntensityMax, 150.0f, 1.0f, 0.0f, 1000.0f);
    binder_->Bind("LightRadius", &config_.lightRadius, 100.0f, 1.0f, 5.0f, 500.0f);
    binder_->Bind("LightHeightOffset", &config_.lightHeightOffset, 10.0f, 0.5f, 0.0f, 50.0f);
    binder_->Bind("VolumetricScattering", &config_.volumetricScattering, 5.0f, 0.1f, 0.0f, 20.0f);

    engine_->GetRendererManager()->SetLightningConfig(config_);
}

void LightningSystem::Update()
{
    float dt = TimeManager::GetInstance()->GetDeltaTime();
    auto lightManager = engine_->GetLightManager();

    // アクティブな落雷ポイントライトの更新（寿命減衰とチカチカの同期）
    if (lightManager)
    {
        for (auto it = activeLights_.begin(); it != activeLights_.end();)
        {
            it->currentDuration -= dt;
            if (it->currentDuration <= 0.0f)
            {
                // 寿命が尽きたら安全に返却
                lightManager->ReturnPointLight(it->lightIndex);
                it = activeLights_.erase(it);
            }
            else
            {
                // 残り時間によるリニアなフェードアウト (1.0 -> 0.0)
                float t = it->currentDuration / it->maxDuration;

                // 雷ポリゴンの明滅のノイズ/サイン波
                float flicker = 1.0f;
                if (config_.flickerSpeed > 0.0f)
                {
                    float totalTime = TimeManager::GetInstance()->GetTotalTime();
                    flicker = std::sin(totalTime * config_.flickerSpeed + it->seed);
                    // サイン波 -1.0 ~ 1.0 を 0.4 ~ 1.2 にマッピング
                    flicker = 0.4f + (flicker + 1.0f) * 0.5f * (1.2f - 0.4f);
                }

                // 最終光度 = 最大輝度 * 寿命フェード * 明滅
                float currentIntensity = config_.lightIntensityMax * t * flicker;

                // ライトのパラメータを更新
                lightManager->UpdatePointLightProperties(
                    it->lightIndex,
                    config_.lightColor,
                    currentIntensity,
                    config_.lightRadius,
                    config_.volumetricScattering
                );

                ++it;
            }
        }
    }

    engine_->GetRendererManager()->UpdateLightnings();
    engine_->GetRendererManager()->SetLightningConfig(config_);
}

void LightningSystem::SpawnStrike(const Vector3& start, const Vector3& end)
{
    TriggerSingleStrike(start, end);
}

void LightningSystem::TriggerSingleStrike(const Vector3& start, const Vector3& end)
{
    // 寿命を Min 〜 Max の間でランダムに決定
    float randomDuration = config_.durationMin +
        (static_cast<float>(rand() % 100) / 100.0f) * (config_.durationMax - config_.durationMin);

    // レンダラーに雷のメッシュ生成を命令
    engine_->GetRendererManager()->SpawnLightning(start, end, randomDuration);

    // 落雷ポイントライトの作成
    auto lightManager = engine_->GetLightManager();
    if (lightManager)
    {
        int freeIndex = lightManager->RequestPointLight();
        if (freeIndex != -1)
        {
            ActiveLight al;
            al.lightIndex = freeIndex;
            al.maxDuration = randomDuration;
            al.currentDuration = randomDuration;
            al.seed = static_cast<float>(rand() % 100);

            // 地面から少し浮かせた位置にライトを配置
            Vector3 lightPos = end + Vector3(0.0f, config_.lightHeightOffset, 0.0f);
            lightManager->UpdatePointLightPosition(freeIndex, lightPos);

            // 初期パラメータ設定
            lightManager->UpdatePointLightProperties(
                freeIndex,
                config_.lightColor,
                config_.lightIntensityMax,
                config_.lightRadius,
                config_.volumetricScattering
            );

            activeLights_.push_back(al);
        }
    }
}

void LightningSystem::DebugDraw()
{
#ifdef IS_DEVELOPMENT
    ImGui::Begin("環境設定");

    if (ImGui::CollapsingHeader("雷エフェクト", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("カラー設定");
        binder_->Draw("CoreColor", "芯の色");
        binder_->Draw("FringeColor", "滲みの色");
        binder_->Draw("LightColor", "落雷ライトの色");

        ImGui::Separator();

        ImGui::Text("発光と形状");
        binder_->Draw("Thickness", "雷全体の太さ");
        binder_->Draw("CoreThickness", "芯の太さ");
        binder_->Draw("CorePower", "芯の鋭さ");
        binder_->Draw("GlowPower", "発光の広がり");
        binder_->Draw("EmissiveIntensity", "ブルームの強さ");
        binder_->Draw("FractalDepth", "フラクタルの分割数");
        binder_->Draw("Displacement", "ジグザグの激しさ");
        binder_->Draw("BranchProbability", "枝分かれの確率");
        binder_->Draw("BranchLengthScale", "枝の長さの割合");

        ImGui::Separator();

        ImGui::Text("寿命と明滅");
        binder_->Draw("DurationMin", "最小表示時間");
        binder_->Draw("DurationMax", "最大表示時間");
        binder_->Draw("FlickerSpeed", "明滅スピード");
        binder_->Draw("FlickerMin", "明滅の最小輝度");
        binder_->Draw("FlickerMax", "明滅の最大輝度");

        ImGui::Separator();

        ImGui::Text("落雷ライト");
        binder_->Draw("LightIntensityMax", "ライト最大輝度");
        binder_->Draw("LightRadius", "ライト影響半径");
        binder_->Draw("LightHeightOffset", "ライトの高さオフセット");
        binder_->Draw("VolumetricScattering", "フォグの散乱強度");

        ImGui::Spacing();

        if (ImGui::Button("テスト落雷", ImVec2(120, 30)))
        {
            SpawnStrike(Vector3(0, 500, 0), Vector3(10, 0, 10));
        }
    }
    ImGui::End();
#endif
}

}