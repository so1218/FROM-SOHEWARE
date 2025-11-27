#include "Model.h"
#include "Engine.h"
#include "TextureHandle.h"

Model::Model(Engine* engine, Camera* camera, ModelData* modelData)
    : engine_(engine), camera_(camera), modelData_(modelData)
{
    materialHandle_ = engine_->materialManager_->CreateMaterial(engine_->graphicsDevice_->GetDevice());

    textureHandle_ = TextureHandle::Get(TextureID::white1x1);
    envMapTextureHandle_ = TextureHandle::Get(TextureID::skyboxCubemap);
}

void Model::SetWorldTransform(const WorldTransform& transform){ transform_ = transform;}
void Model::SetUVTransform(const WorldTransform& uvTransform)
{
    uvTransform_ = uvTransform;
    uvTransform_.UpdateMatrix();
    materialHandle_.materialData->uvTransform = uvTransform_.matWorld_;
}
void Model::SetTextureHandle(uint32_t handle){ textureHandle_ = handle;}
void Model::SetEnvironmentMapHandle(uint32_t handle){ envMapTextureHandle_ = handle;}
void Model::SetColor(uint32_t color){ color_ = color;}
void Model::SetCamera(Camera* camera){ camera_ = camera;}
void Model::SetEnableOutline(bool enable) { enableOutline_ = enable; }
void Model::SetOutlineWidth(float width) { outlineWidth_ = width; }
void Model::SetOutlineColor(const Vector4& color) { outlineColor_ = color; }

void Model::Draw()
{
    transform_.UpdateMatrix();
    engine_->renderer_->SubmitModel(
        transform_,
        *camera_,
        *modelData_,
        textureHandle_,
        envMapTextureHandle_,
        color_,
        materialHandle_,
        enableOutline_,
        outlineWidth_,
        outlineColor_
    );
}
