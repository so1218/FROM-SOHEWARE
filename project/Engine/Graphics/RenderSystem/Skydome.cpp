#include "pch.h"
#include "Skydome.h"
#include "Engine.h"
#include "TextureManager.h"
#include "Camera.h"
#include "PropertyBinder.h"
#include "EnvironmentManager.h"

namespace FE
{

Skydome::Skydome(Engine* engine)
    : engine_(engine)
{
    transform_.scale_ = { 1.0f, 1.0f, 1.0f };
    transform_.rotation_ = { 0.0f, 0.0f, 0.0f };
    transform_.translation_ = { 0.0f, 0.0f, 0.0f };

    // デフォルトテクスチャの読み込み
    skyCubeHandle_ = TextureManager::GetInstance().Get("black_cube");
    cloudNoiseHandle_ = TextureManager::GetInstance().Get("noise_59");

    binder_ = std::make_unique<PropertyBinder>(engine_, "Skydome");
}

void Skydome::Initialize()
{
    // バインド処理
    binder_->Bind("WindVelocity", &weatherData_.windVelocity, { 0.006f, 0.003f }, 0.001f, -0.1f, 0.1f);
    binder_->Bind("CloudScale", &weatherData_.cloudScale, 0.3f, 0.01f, 0.01f, 2.0f);
    binder_->Bind("SkyGradientExp", &weatherData_.skyGradientExponent, 0.6f, 0.01f, 0.1f, 5.0f);
    binder_->Bind("CloudBumpScale", &weatherData_.cloudBumpScale, 0.15f, 0.005f, 0.01f, 1.0f);
    binder_->Bind("CloudEdgeSoftness", &weatherData_.cloudEdgeSoftness, 0.15f, 0.01f, 0.01f, 0.5f);
    binder_->Bind("CloudAbsorption", &weatherData_.cloudAbsorption, 0.7f, 0.01f, 0.0f, 1.0f);
}

void Skydome::Update()
{
    // 太陽の向きを取得
    weatherData_.sunDirection = EnvironmentManager::GetInstance()->GetSunDirection();

    // 現在ブレンド済みの環境パラメータを取得し、適用
    const TimeOfDayProfile& profile = EnvironmentManager::GetInstance()->GetCurrentProfile();

    weatherData_.zenithColor = profile.zenithColor;
    weatherData_.horizonColor = profile.horizonColor;
    weatherData_.groundColor = profile.groundColor;
    weatherData_.cloudAmbientColor = profile.cloudAmbientColor;
    weatherData_.sunAtmosphereGlow = profile.sunAtmosphereGlow;

    // 天候プロファイルを取得し、雲の量を上書きする
    const WeatherProfile& weather = EnvironmentManager::GetInstance()->GetCurrentWeatherProfile();

    weatherData_.cloudCoverage.x = weather.cloudCoverageMin;
    weatherData_.cloudCoverage.y = weather.cloudCoverageMax;
    weatherData_.cloudShadowDensity = weather.cloudShadowDensity;
}

void Skydome::SetSkyCubeTexture(const std::string& textureName) {
    skyCubeHandle_ = TextureManager::GetInstance().Get(textureName);
}
void Skydome::SetCloudNoiseTexture(const std::string& textureName) {
    cloudNoiseHandle_ = TextureManager::GetInstance().Get(textureName);
}
void Skydome::SetColor(uint32_t color) { color_ = color; }
WorldTransform& Skydome::GetTransform() { return transform_; }

void Skydome::Draw()
{
    transform_.UpdateMatrix();

    // 2つのテクスチャハンドルを渡してSubmit
    engine_->GetRendererManager()->SubmitSkydome(
        transform_,
        color_,
        skyCubeHandle_,
        cloudNoiseHandle_,
        weatherData_
    );
}

void Skydome::DebugDraw()
{
#ifdef IS_DEVELOPMENT
    ImGui::Begin("環境設定");

    if (ImGui::CollapsingHeader("スカイドーム", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("雲の形状・アニメーション設定");
        binder_->Draw("CloudScale", "雲のサイズ");
        binder_->Draw("WindVelocity", "風向きと強さ");
        binder_->Draw("CloudEdgeSoftness", "雲の輪郭の柔らかさ");
        binder_->Draw("CloudBumpScale", "雲の凹凸の強さ (モクモク感)");

        ImGui::Separator();

        ImGui::Text("雲のライティング・陰影設定");
        binder_->Draw("CloudAbsorption", "雲の厚みによる光の遮蔽率");

        ImGui::Separator();
    }

    ImGui::End();
#endif
}

}