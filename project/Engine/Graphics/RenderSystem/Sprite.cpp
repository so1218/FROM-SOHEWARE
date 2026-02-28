#include "Sprite.h"
#include "Engine.h"
#include "TextureManager.h"

Sprite::Sprite(Engine* engine)
    : engine_(engine)
{
    uvTransform_.scale_ = { 1.0f, 1.0f, 1.0f };
    uvTransform_.rotation_ = { 0.0f, 0.0f, 0.0f };
    uvTransform_.translation_ = { 0.0f, 0.0f, 0.0f };

    // 名前初期化
    textureName_ = "white1x1";
    dissolveTextureName_ = "white1x1";

    auto& texManager = TextureManager::GetInstance();

    // ハンドル取得
    textureHandle_ = texManager.Get(textureName_);
    dissolveTextureHandle_ = texManager.Get(dissolveTextureName_);

    materialHandle_ = engine_->materialManager_->CreateMaterial(engine_->graphicsDevice_->GetDevice());
}

void Sprite::SetTexture(const std::string& textureName)
{
    textureName_ = textureName; 
    textureHandle_ = TextureManager::GetInstance().Get(textureName_);
}

void Sprite::SetDissolveTexture(const std::string& textureName)
{
    dissolveTextureName_ = textureName;
    dissolveTextureHandle_ = TextureManager::GetInstance().Get(dissolveTextureName_);
}

void Sprite::SetColor(const Vector4& color) { color_ = Math::ColorVectorToUint32(color); }

void Sprite::Draw()
{
    if (!isVisible_)
    {
        return;
    }

    engine_->rendererManager_->SubmitSprite(
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