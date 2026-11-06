#include "pch.h"
#include "LightningSystem.h"
#include "Engine.h"
#include "PropertyBinder.h"
#include "TimeManager.h"

namespace FS
{

LightningSystem::LightningSystem(Engine* engine)
    : engine_(engine)
    , randomEngine_(std::random_device{}())
{
    binder_ = std::make_unique<PropertyBinder>(engine_, "Lightning");
}

LightningSystem::~LightningSystem()
{
}

void LightningSystem::Initialize()
{
    // 見た目
    binder_->BindColor("CoreColor", &config_.coreColor, { 1.0f, 1.0f, 1.0f });
    binder_->BindColor("FringeColor", &config_.fringeColor, { 0.1f, 0.5f, 1.0f });
    binder_->Bind("Thickness", &config_.thickness, 0.5f, 0.1f, 0.05f, 10.0f);
    binder_->Bind("CoreThickness", &config_.coreThickness, 0.15f, 0.01f, 0.01f, 1.0f);
    binder_->Bind("CorePower", &config_.corePower, 4.0f, 0.1f, 0.1f, 20.0f);
    binder_->Bind("GlowPower", &config_.glowPower, 2.5f, 0.1f, 0.1f, 10.0f);
    binder_->Bind("EmissiveIntensity", &config_.emissiveIntensity, 50.0f, 1.0f, 1.0f, 1000.0f);

    // 形
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

    // 落ちた場所を照らす光
    binder_->BindColor("LightColor", &config_.lightColor, { 0.7f, 0.85f, 1.0f, 1.0f });
    binder_->Bind("LightIntensityMax", &config_.lightIntensityMax, 150.0f, 1.0f, 0.0f, 1000.0f);
    binder_->Bind("LightRadius", &config_.lightRadius, 100.0f, 1.0f, 5.0f, 500.0f);
    binder_->Bind("LightHeightOffset", &config_.lightHeightOffset, 10.0f, 0.5f, 0.0f, 50.0f);
    binder_->Bind("VolumetricScattering", &config_.volumetricScattering, 5.0f, 0.1f, 0.0f, 20.0f);

    strikes_.clear();
}

void LightningSystem::Update()
{
    const float deltaTime = TimeManager::GetInstance()->GetDeltaTime();
    auto* rendererManager = engine_->GetRendererManager();
    auto* lightManager = engine_->GetLightManager();

    // エディタで変えた見た目をすぐ反映する
    rendererManager->SetLightningConfig(config_);

    for (auto it = strikes_.begin(); it != strikes_.end(); )
    {
        it->remainingTime -= deltaTime;
        if (it->remainingTime <= 0.0f)
        {
            it = strikes_.erase(it);
            continue;
        }

        // 雷の板と落ちた場所の光は、同じ明るさで消えて同じように明滅する
        const float fade = it->remainingTime / it->duration;
        const float brightness = fade * CalculateFlicker(it->flickerPhase);

        rendererManager->SubmitLightning(it->paths, brightness);
        lightManager->SubmitPointLight(it->lightPosition, config_.lightColor,
            config_.lightIntensityMax * brightness, config_.lightRadius, config_.volumetricScattering);

        ++it;
    }
}

float LightningSystem::CalculateFlicker(float flickerPhase) const
{
    if (config_.flickerSpeed <= 0.0f)
    {
        return 1.0f;
    }

    // 1秒に flickerSpeed 回、明るさを変える
    const float step = std::floor(TimeManager::GetInstance()->GetTotalTime() * config_.flickerSpeed + flickerPhase);
    const float noise = std::sin(step * 12.9898f) * 43758.5453f;
    return std::lerp(config_.flickerMin, config_.flickerMax, noise - std::floor(noise));
}

void LightningSystem::SpawnStrike(const Vector3& start, const Vector3& end)
{
    std::uniform_real_distribution<float> durationDistribution(config_.durationMin, config_.durationMax);
    std::uniform_real_distribution<float> phaseDistribution(0.0f, 100.0f);

    Strike strike;
    strike.duration = durationDistribution(randomEngine_);
    strike.remainingTime = strike.duration;
    strike.flickerPhase = phaseDistribution(randomEngine_);
    strike.lightPosition = end + Vector3(0.0f, config_.lightHeightOffset, 0.0f);

    LightningPath trunk;
    trunk.points.push_back(start);
    GenerateFractalPath(strike, trunk.points, start, end, config_.fractalDepth, config_.displacement, 1.0f);
    strike.paths.insert(strike.paths.begin(), std::move(trunk));

    strikes_.push_back(std::move(strike));
}

void LightningSystem::GenerateFractalPath(Strike& strike, std::vector<Vector3>& points,
    const Vector3& start, const Vector3& end, int depth, float displacement, float thicknessScale)
{
    if (depth == 0)
    {
        points.push_back(end);
        return;
    }

    std::uniform_real_distribution<float> offsetDistribution(-0.5f, 0.5f);
    std::uniform_real_distribution<float> chanceDistribution(0.0f, 1.0f);

    // 縦にずらすと雷が上に戻って見えるので、縦は控えめにする
    Vector3 middle = (start + end) * 0.5f + Vector3(
        offsetDistribution(randomEngine_) * displacement,
        offsetDistribution(randomEngine_) * displacement * 0.05f,
        offsetDistribution(randomEngine_) * displacement);

    GenerateFractalPath(strike, points, start, middle, depth - 1, displacement * 0.5f, thicknessScale);

    // 枝は途中から斜めに伸ばし、幹の半分の太さにする
    if (chanceDistribution(randomEngine_) < config_.branchProbability)
    {
        Vector3 branchOffset(offsetDistribution(randomEngine_), offsetDistribution(randomEngine_) * 0.5f, offsetDistribution(randomEngine_));
        Vector3 branchEnd = middle + (end - start) * config_.branchLengthScale + branchOffset * (displacement * 2.0f);

        LightningPath branch;
        branch.thicknessScale = thicknessScale * 0.5f;
        branch.points.push_back(middle);
        GenerateFractalPath(strike, branch.points, middle, branchEnd, depth - 1, displacement * 0.5f, branch.thicknessScale);
        strike.paths.push_back(std::move(branch));
    }

    GenerateFractalPath(strike, points, middle, end, depth - 1, displacement * 0.5f, thicknessScale);
}

// 呼び出し側が開いたウィンドウやタブの中に書く
void LightningSystem::DebugGui()
{
    if (ImGui::CollapsingHeader("雷の見た目"))
    {
        ImGui::SeparatorText("見た目");
        binder_->Draw("CoreColor", "芯の色");
        binder_->Draw("FringeColor", "にじみの色");
        binder_->Draw("Thickness", "雷全体の太さ");
        binder_->Draw("CoreThickness", "芯の太さ");
        binder_->Draw("CorePower", "芯の鋭さ");
        binder_->Draw("GlowPower", "にじみの広がり");
        binder_->Draw("EmissiveIntensity", "ブルームの強さ");

        ImGui::SeparatorText("形");
        binder_->Draw("FractalDepth", "分割の回数");
        binder_->Draw("Displacement", "ジグザグの激しさ");
        binder_->Draw("BranchProbability", "枝分かれの確率");
        binder_->Draw("BranchLengthScale", "枝の長さの割合");

        ImGui::SeparatorText("寿命と明滅");
        binder_->Draw("DurationMin", "最短の表示時間");
        binder_->Draw("DurationMax", "最長の表示時間");
        binder_->Draw("FlickerSpeed", "1秒の明滅回数");
        binder_->Draw("FlickerMin", "明滅の最小");
        binder_->Draw("FlickerMax", "明滅の最大");

        ImGui::SeparatorText("落ちた場所の光");
        binder_->Draw("LightColor", "光の色");
        binder_->Draw("LightIntensityMax", "最大の明るさ");
        binder_->Draw("LightRadius", "届く半径");
        binder_->Draw("LightHeightOffset", "地面からの高さ");
        binder_->Draw("VolumetricScattering", "霧に当たる強さ");
    }
}

}
