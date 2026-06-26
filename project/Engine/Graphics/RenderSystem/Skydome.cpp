#include "pch.h"
#include "Skydome.h"
#include "Engine.h"
#include "TextureManager.h"
#include "Camera.h"

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
        cloudNoiseHandle_
    );
}

}