#include "Model.h"
#include "Engine.h"

Model::Model(Engine* engine, Camera* camera, ModelData* modelData)
    : engine_(engine), camera_(camera), modelData_(modelData)
{
    materialHandle_ = engine_->materialManager_->CreateMaterial(engine_->graphicsDevice_->GetDevice());
}

void Model::SetWorldTransform(const WorldTransform& transform)
{
    transform_ = transform;
}

void Model::SetUVTransform(const WorldTransform& uvTransform)
{
    uvTransform_ = uvTransform;
    uvTransform_.UpdateMatrix();
    materialHandle_.materialData->uvTransform = uvTransform_.matWorld_;
}

void Model::SetTextureHandle(uint32_t handle)
{
    textureHandle_ = handle;
}

void Model::SetColor(uint32_t color)
{
    color_ = color;
}

void Model::SetCamera(Camera* camera)
{
    camera_ = camera;
}

void Model::Draw()
{
    transform_.UpdateMatrix();

    engine_->renderer_->DrawModel(transform_, *camera_, *modelData_, textureHandle_, color_, materialHandle_);
}
