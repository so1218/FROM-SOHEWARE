#include "pch.h"
#include "Skydome.h"
#include "Engine.h"
#include "TextureManager.h"
#include "PropertyBinder.h"
#include "EnvironmentManager.h"
#include "EnvironmentWindow.h"

namespace FS
{

Skydome::Skydome(Engine* engine)
    : engine_(engine)
{
    cloudNoiseHandle_ = TextureManager::GetInstance().Get("noise_59");

    binder_ = std::make_unique<PropertyBinder>(engine_, "Skydome");
}

Skydome::~Skydome() = default;

void Skydome::Initialize()
{
    binder_->Bind("WindVelocity", &atmosphereSkyData_.windVelocity, { 0.006f, 0.003f }, 0.001f, -0.1f, 0.1f);
    binder_->Bind("CloudScale", &atmosphereSkyData_.cloudScale, 0.3f, 0.01f, 0.01f, 2.0f);
    binder_->Bind("SkyGradientExp", &atmosphereSkyData_.skyGradientExponent, 0.6f, 0.01f, 0.1f, 5.0f);
    binder_->Bind("CloudBumpScale", &atmosphereSkyData_.cloudBumpScale, 0.15f, 0.005f, 0.01f, 1.0f);
    binder_->Bind("CloudEdgeSoftness", &atmosphereSkyData_.cloudEdgeSoftness, 0.15f, 0.01f, 0.01f, 0.5f);
    binder_->Bind("CloudAbsorption", &atmosphereSkyData_.cloudAbsorption, 0.7f, 0.01f, 0.0f, 1.0f);

    binder_->Bind("StarDensity", &atmosphereSkyData_.starDensity, 0.06f, 0.005f, 0.0f, 1.0f);
    binder_->Bind("StarGridSize", &atmosphereSkyData_.starGridSize, 250.0f, 1.0f, 10.0f, 2000.0f);
    binder_->Bind("StarSize", &atmosphereSkyData_.starSize, 0.08f, 0.005f, 0.01f, 0.3f);
    binder_->Bind("StarBrightness", &atmosphereSkyData_.starBrightness, 8.0f, 0.1f, 0.0f, 100.0f);
    binder_->Bind("StarTwinkleSpeed", &atmosphereSkyData_.starTwinkleSpeed, 1.5f, 0.05f, 0.0f, 20.0f);
    binder_->Bind("StarTwinkleAmount", &atmosphereSkyData_.starTwinkleAmount, 0.4f, 0.01f, 0.0f, 1.0f);
}

void Skydome::Update()
{
    atmosphereSkyData_.sunDirection = EnvironmentManager::GetInstance()->GetSunDirection();

    const TimeOfDayProfile& profile = EnvironmentManager::GetInstance()->GetCurrentProfile();

    atmosphereSkyData_.zenithColor = profile.zenithColor;
    atmosphereSkyData_.horizonColor = profile.horizonColor;
    atmosphereSkyData_.sunAtmosphereGlow = profile.sunAtmosphereGlow;

    const WeatherProfile& weather = EnvironmentManager::GetInstance()->GetCurrentWeatherProfile();
    atmosphereSkyData_.cloudThreshold = weather.cloudThreshold;
    atmosphereSkyData_.cloudShadowBrightness = weather.cloudShadowBrightness;
}

void Skydome::SetCloudNoiseTexture(const std::string& textureName)
{
    cloudNoiseHandle_ = TextureManager::GetInstance().Get(textureName);
}

void Skydome::Draw()
{
    engine_->GetRendererManager()->SubmitSkydome(cloudNoiseHandle_, atmosphereSkyData_);
}

void Skydome::DebugGui()
{
    if (BeginEnvironmentTab("空"))
    {
        ImGui::Text("雲の形状・アニメーション設定");
        binder_->Draw("CloudScale", "雲のサイズ");
        binder_->Draw("WindVelocity", "風向きと強さ");
        binder_->Draw("CloudEdgeSoftness", "雲の輪郭の柔らかさ");
        binder_->Draw("CloudBumpScale", "雲の凹凸の強さ");
        binder_->Draw("SkyGradientExp", "空のグラデーションのかかり方");

        ImGui::Separator();

        ImGui::Text("雲のライティング・陰影設定");
        binder_->Draw("CloudAbsorption", "雲の厚みによる光の遮蔽率");

        ImGui::SeparatorText("星 (太陽が沈むと見える)");
        binder_->Draw("StarDensity", "星を置くマスの割合");
        binder_->Draw("StarGridSize", "空を分けるマスの細かさ (立方体 1 面の 1 辺)");
        binder_->Draw("StarSize", "星の大きさ (1 マスに対する割合)");
        binder_->Draw("StarBrightness", "明るさ");
        binder_->Draw("StarTwinkleSpeed", "瞬きの速さ");
        binder_->Draw("StarTwinkleAmount", "瞬きで明るさが変わる割合");

        EndEnvironmentTab();
    }
}

}