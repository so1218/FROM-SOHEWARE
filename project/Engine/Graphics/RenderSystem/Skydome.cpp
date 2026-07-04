#include "pch.h"
#include "Skydome.h"
#include "Engine.h"
#include "TextureManager.h"
#include "Camera.h"
#include "PropertyBinder.h"

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
    binder_->Bind("CloudCoverage", &weatherData_.cloudCoverage, { 0.35f, 0.7f }, 0.01f, 0.0f, 1.0f);
    binder_->Bind("WindVelocity", &weatherData_.windVelocity, { 0.006f, 0.003f }, 0.001f, -0.1f, 0.1f);
    binder_->Bind("CloudScale", &weatherData_.cloudScale, 0.3f, 0.01f, 0.01f, 2.0f);
    binder_->Bind("CloudShadowDensity", &weatherData_.cloudShadowDensity, 0.6f, 0.01f, 0.0f, 1.0f);
    binder_->Bind("SkyGradientExp", &weatherData_.skyGradientExponent, 0.6f, 0.01f, 0.1f, 5.0f);
    binder_->Bind("SunAtmoGlow", &weatherData_.sunAtmosphereGlow, 0.5f, 0.01f, 0.0f, 2.0f);
    binder_->BindColor("ZenithColor", &weatherData_.zenithColor, { 0.05f, 0.15f, 0.4f });
    binder_->BindColor("HorizonColor", &weatherData_.horizonColor, { 0.4f, 0.6f, 0.8f });
    binder_->BindColor("GroundColor", &weatherData_.groundColor, { 0.2f, 0.2f, 0.2f });
    binder_->Bind("CloudBumpScale", &weatherData_.cloudBumpScale, 0.15f, 0.005f, 0.01f, 1.0f);
    binder_->Bind("CloudEdgeSoftness", &weatherData_.cloudEdgeSoftness, 0.15f, 0.01f, 0.01f, 0.5f);
    binder_->Bind("CloudAbsorption", &weatherData_.cloudAbsorption, 0.7f, 0.01f, 0.0f, 1.0f);
    binder_->BindColor("CloudAmbientColor", &weatherData_.cloudAmbientColor, { 0.08f, 0.12f, 0.2f });
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
        binder_->Draw("CloudCoverage", "雲の発生量 (下限/上限)");
        binder_->Draw("CloudScale", "雲のサイズ");
        binder_->Draw("WindVelocity", "風向きと強さ");
        binder_->Draw("CloudEdgeSoftness", "雲の輪郭の柔らかさ");
        binder_->Draw("CloudBumpScale", "雲の凹凸の強さ (モクモク感)");

        ImGui::Separator();

        ImGui::Text("雲のライティング・陰影設定");
        binder_->Draw("CloudShadowDensity", "雲全体の影の濃さ");
        binder_->Draw("CloudAbsorption", "雲の厚みによる光の遮蔽率");
        binder_->Draw("CloudAmbientColor", "雲の環境光 (影の色)");

        ImGui::Separator();

        ImGui::Text("空のグラデーション・大気設定");
        binder_->Draw("ZenithColor", "天頂の色");
        binder_->Draw("HorizonColor", "地平線の色");
        binder_->Draw("GroundColor", "地面 (地平線下) の色");
        binder_->Draw("SkyGradientExp", "グラデーションのカーブ");
        binder_->Draw("SunAtmoGlow", "大気散乱 (太陽周辺の明るさ)");

        ImGui::Separator();
    }

    ImGui::End();
#endif
}

}