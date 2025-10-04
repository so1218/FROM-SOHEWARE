#include "Model.h"
#include "Engine.h"

Model::Model(Engine* engine, Camera* camera, std::unique_ptr<ModelData> modelData)
    : engine_(engine), camera_(camera), modelData_(std::move(modelData)) 
{
}

void Model::SetWorldTransform(const WorldTransform& transform)
{
    transform_ = transform;
}

void Model::SetUVTransform(const WorldTransform& uvTransform) 
{
    uvTransform_ = uvTransform;
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

    engine_->DrawModel(transform_, *camera_, *modelData_, textureHandle_, color_);
}

void Model::DrawWithUV()
{
    transform_.UpdateMatrix();
    uvTransform_.UpdateMatrix();

    engine_->DrawModel(transform_, *camera_, *modelData_, textureHandle_, color_, uvTransform_);
}