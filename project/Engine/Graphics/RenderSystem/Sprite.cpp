#include "pch.h"
#include "Sprite.h"
#include "Engine.h"
#include "TextureManager.h"

namespace FS
{

Sprite::Sprite(Engine* engine)
    : engine_(engine)
{
    uvTransform_.scale_ = { 1.0f, 1.0f, 1.0f };
    uvTransform_.rotation_ = { 0.0f, 0.0f, 0.0f };
    uvTransform_.translation_ = { 0.0f, 0.0f, 0.0f };

    textureName_ = "white1x1";
    dissolveTextureName_ = "white1x1";

    auto& textureManager = TextureManager::GetInstance();
    textureHandle_ = textureManager.Get(textureName_);
    dissolveTextureHandle_ = textureManager.Get(dissolveTextureName_);
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

void Sprite::SetColor(const Vector4& color)
{
    color_ = Math::ColorVectorToUint32(color);
}

void Sprite::Draw()
{
    if (!isVisible_)
    {
        return;
    }

    SpriteInstance instance{};

    // 位置・大きさ
    instance.position = position_;
    instance.size = size_;
    instance.pivot = anchorPoint_;
    instance.rotation = rotation_;

    // 見た目
    instance.textureIndex = textureHandle_;
    instance.color = Math::Uint32ToColorVector(color_);
    instance.uvRect = uvRect_;

    // UV 変換 
    Matrix4x4 uvTransformMatrix =
        Matrix4x4::MakeScale(uvTransform_.scale_) *
        Matrix4x4::MakeRotateZ(uvTransform_.rotation_.z) *
        Matrix4x4::MakeTranslate(uvTransform_.translation_);
    instance.uvMatrix = Vector4(
        uvTransformMatrix.m[0][0], uvTransformMatrix.m[0][1],
        uvTransformMatrix.m[1][0], uvTransformMatrix.m[1][1]);
    instance.uvTranslate = Vector2(uvTransformMatrix.m[3][0], uvTransformMatrix.m[3][1]);

    // ディゾルブ
    instance.enableDissolve = (material_.enableDissolve != 0) ? 1u : 0u;
    instance.dissolveTextureIndex = dissolveTextureHandle_;
    instance.dissolveThreshold = material_.dissolveThreshold;
    instance.edgeColor = material_.edgeColor;
    instance.edgeWidth = material_.edgeWidth;
    instance.edgeIntensity = material_.edgeIntensity;
    instance.emissiveIntensity = material_.emissiveIntensity;

    engine_->GetRendererManager()->SubmitSprite(instance, layerOrder_);
}

}