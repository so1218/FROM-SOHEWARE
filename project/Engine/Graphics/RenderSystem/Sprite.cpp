#include "Sprite.h"
#include "Engine.h"
#include "TextureHandle.h"

Sprite::Sprite(Engine* engine)
    : engine_(engine)
{
    uvTransform_.scale_ = { 1.0f, 1.0f, 1.0f };
    uvTransform_.rotation_ = { 0.0f, 0.0f, 0.0f };
    uvTransform_.translation_ = { 0.0f, 0.0f, 0.0f };

    textureHandle_ = TextureHandle::Get(TextureID::white1x1);
    materialHandle_ = engine_->materialManager_->CreateMaterial(engine_->graphicsDevice_->GetDevice());
    materialHandle_.materialData->emissiveIntensity = 5.0f;
}

void Sprite::SetPosition(const Vector2& position) { position_ = position; }
void Sprite::SetSize(const Vector2& size) { size_ = size; }
void Sprite::SetRotation(float rotation) { rotation_ = rotation; }
void Sprite::SetColor(uint32_t color) { color_ = color; }
void Sprite::SetTextureHandle(uint32_t handle) { textureHandle_ = handle; }
void Sprite::SetUVTransform(const WorldTransform& uv) { uvTransform_ = uv; }
void Sprite::SetLayerOrder(int order) { layerOrder_ = order; }

Vector2& Sprite::GetPosition() { return position_; }
Vector2& Sprite::GetSize() { return size_; }
float& Sprite::GetRotation() { return rotation_; }
WorldTransform& Sprite::GetUVTransform() { return uvTransform_; }
int Sprite::GetLayerOrder() const { return layerOrder_; }

void Sprite::Draw()
{
    if (!isVisible_)
    {
        return;
    }

    uvTransform_.UpdateMatrix();
    engine_->renderer_->SubmitSprite(
        position_,
        size_,
        rotation_,
        color_,
        uvTransform_,
        textureHandle_,
        layerOrder_,
        materialHandle_
    );
}
