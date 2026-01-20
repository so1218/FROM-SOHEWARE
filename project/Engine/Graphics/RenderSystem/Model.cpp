#include "Model.h"
#include "Engine.h"
#include "MaterialManager.h"

Model::Model(Engine* engine, const ModelData* modelData)
    : engine_(engine), modelData_(modelData)
{
    materialHandle_ = engine_->materialManager_->CreateMaterial(engine_->graphicsDevice_->GetDevice());

    textureHandle_ = TextureHandle::Get(TextureID::white1x1);
    envMapTextureHandle_ = TextureHandle::Get(TextureID::skyboxCubemap);
    toonRampHandle_ = TextureHandle::Get(TextureID::toonRamp);
    dissolveTextureHandle_ = TextureHandle::Get(TextureID::white1x1);
    normalMapHandle_ = TextureHandle::Get(TextureID::white1x1);
}

void Model::SetWorldTransform(const WorldTransform& transform) { transform_ = transform; }
void Model::SetUVTransform(const WorldTransform& uvTransform)
{
    uvTransform_ = uvTransform;
    uvTransform_.UpdateMatrix();
    materialHandle_.materialData->uvTransform = uvTransform_.matWorld_;
}
void Model::SetTexture(TextureID textureID) { textureHandle_ = TextureHandle::Get(textureID); }
void Model::SetEnvironmentMapTexture(TextureID textureID) { envMapTextureHandle_ = TextureHandle::Get(textureID); }
void Model::SetColor(uint32_t color) { color_ = color; }
void Model::SetEnableOutline(bool enable) { enableOutline_ = enable; }
void Model::SetOutlineWidth(float width) { outlineWidth_ = width; }
void Model::SetOutlineColor(const Vector4& color) { outlineColor_ = color; }
void Model::SetRenderGroup(RenderGroup group) { renderGroup_ = group; }

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
