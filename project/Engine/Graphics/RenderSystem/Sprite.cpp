#include "Sprite.h"
#include "Engine.h"

Sprite::Sprite(Engine* engine)
    : engine_(engine)
{
    uvTransform_.scale_ = { 1.0f, 1.0f, 1.0f };
    uvTransform_.rotation_ = { 0.0f, 0.0f, 0.0f };
    uvTransform_.translation_ = { 0.0f, 0.0f, 0.0f };
}

void Sprite::SetPosition(const Vector2& position) { position_ = position; }
void Sprite::SetSize(const Vector2& size) { size_ = size; }
void Sprite::SetRotation(float rotation) { rotation_ = rotation; }
void Sprite::SetColor(uint32_t color) { color_ = color; }
void Sprite::SetTextureHandle(uint32_t handle) { textureHandle_ = handle; }
void Sprite::SetUVTransform(const WorldTransform& uv) { uvTransform_ = uv; }

Vector2& Sprite::GetPosition() { return position_; }
Vector2& Sprite::GetSize() { return size_; }
float& Sprite::GetRotation() { return rotation_; }
WorldTransform& Sprite::GetUVTransform() { return uvTransform_; }

void Sprite::Draw() 
{
    uvTransform_.UpdateMatrix(); 
    engine_->DrawSprite(position_, size_, rotation_, color_, uvTransform_, textureHandle_);
}
