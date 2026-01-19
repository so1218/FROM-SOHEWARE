#include "Skybox.h"
#include "Engine.h"
#include "Renderer.h"
#include "Camera.h"

Skybox::Skybox(Engine* engine)
	: engine_(engine)
{
    // トランスフォームを初期化
    transform_.scale_ = { 1.0f, 1.0f, 1.0f };
    transform_.rotation_ = { 0.0f, 0.0f, 0.0f };
    transform_.translation_ = { 0.0f, 0.0f, 0.0f };
    cubeTextureHandle_ = TextureHandle::Get(TextureID::skyboxCubemapBlack);
}

void Skybox::SetCubeTexture(TextureID textureID)
{
    cubeTextureHandle_ = TextureHandle::Get(textureID);
}

void Skybox::SetColor(uint32_t color)
{
    color_ = color;
}

WorldTransform& Skybox::GetTransform()
{
    return transform_;
}

void Skybox::Draw()
{
    // トランスフォーム行列を更新
    transform_.UpdateMatrix();

    engine_->renderer_->SubmitSkybox(
        transform_,
        color_,
        cubeTextureHandle_
    );
}