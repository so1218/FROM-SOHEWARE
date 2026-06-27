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
        binder_->Draw("CloudCoverage", "雲の発生量 (Min/Max)");
        binder_->Draw("WindVelocity", "風向きと強さ");
        binder_->Draw("CloudScale", "雲のスケール");
        binder_->Draw("CloudShadowDensity", "雲の影の濃さ");

        ImGui::Separator();
    }

    ImGui::End();
#endif
}

}