#include "Sprite.h"
#include "Engine.h"
#include "TextureHandle.h"

Sprite::Sprite(Engine* engine)
    : engine_(engine)
{
    uvTransform_.scale_ = { 1.0f, 1.0f, 1.0f };
    uvTransform_.rotation_ = { 0.0f, 0.0f, 0.0f };
    uvTransform_.translation_ = { 0.0f, 0.0f, 0.0f };

    // 初期テクスチャ設定
    textureHandle_ = TextureHandle::Get(TextureID::white1x1);
    dissolveTextureHandle_ = TextureHandle::Get(TextureID::white1x1);

    materialHandle_ = engine_->materialManager_->CreateMaterial(engine_->graphicsDevice_->GetDevice());
}

void Sprite::SetTexture(TextureID id) { textureHandle_ = TextureHandle::Get(id); }

void Sprite::SetDissolveTexture(TextureID textureID) { dissolveTextureHandle_ = TextureHandle::Get(textureID); }

void Sprite::SetColor(const Vector4& color) { color_ = Math::ColorVectorToUint32(color); }

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
        anchorPoint_,
        uvTransform_,
        textureHandle_,
        dissolveTextureHandle_,
        layerOrder_,
        materialHandle_
    );
}