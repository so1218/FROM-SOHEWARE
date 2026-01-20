#include "Model.h"
#include "Engine.h"
#include "MaterialManager.h"

Model::Model(Engine* engine, const ModelData* modelData)
    : engine_(engine), modelData_(modelData)
{
    materialHandle_ = engine_->materialManager_->CreateMaterial(engine_->graphicsDevice_->GetDevice());

    // 初期テクスチャ設定
    textureHandle_ = TextureHandle::Get(TextureID::white1x1);
    envMapTextureHandle_ = TextureHandle::Get(TextureID::skyboxCubemap);
    toonRampHandle_ = TextureHandle::Get(TextureID::toonRamp);
    dissolveTextureHandle_ = TextureHandle::Get(TextureID::white1x1);
    normalMapHandle_ = TextureHandle::Get(TextureID::white1x1);
}

void Model::SetUVTransform(const WorldTransform& uvTransform)
{
    uvTransform_ = uvTransform;
    uvTransform_.UpdateMatrix();
    materialHandle_.materialData->uvTransform = uvTransform_.matWorld_;
}

void Model::SetTexture(TextureID textureID) { textureHandle_ = TextureHandle::Get(textureID); }
void Model::SetEnvironmentMapTexture(TextureID textureID) { envMapTextureHandle_ = TextureHandle::Get(textureID); }
void Model::SetToonRampTexture(TextureID textureID) { toonRampHandle_ = TextureHandle::Get(textureID); }
void Model::SetDissolveTexture(TextureID textureID) { dissolveTextureHandle_ = TextureHandle::Get(textureID); }
void Model::SetNormalMapTexture(TextureID textureID) { normalMapHandle_ = TextureHandle::Get(textureID); }
void Model::SetColor(const Vector4& color) { color_ = Math::ColorVectorToUint32(color); }
void Model::SetOutlineColor(uint32_t color) { outlineColor_ = Math::Uint32ToColorVector(color); }

void Model::Draw()
{
    transform_.UpdateMatrix();

    engine_->renderer_->SubmitModel(
        transform_,
        *modelData_,
        textureHandle_,
        envMapTextureHandle_,
        toonRampHandle_,
        dissolveTextureHandle_,
        normalMapHandle_,
        color_,
        materialHandle_,
        blendMode_,
        enableOutline_,
        outlineWidth_,
        outlineColor_,
        renderGroup_
    );
}