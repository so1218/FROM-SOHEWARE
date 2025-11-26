#include "Grid.h"
#include "TextureHandle.h"

Grid::Grid(Engine* engine, Camera* camera, ModelData* modelData)
    : engine_(engine), camera_(camera), modelData_(modelData)
{
    // デフォルト
    textureHandle_ = TextureHandle::Get(TextureID::white1x1);
    materialHandle_ = engine_->materialManager_->CreateMaterial(engine_->graphicsDevice_->GetDevice());
    transform_.scale_ = { 10000.0f, 1.0f,10000.0f };
}

void Grid::SetWorldTransform(const WorldTransform& transform)
{
    transform_ = transform;
}
void Grid::SetColor(uint32_t color)
{
    color_ = color;
}
void Grid::SetCamera(Camera* camera)
{
    camera_ = camera;
}
void Grid::SetTextureHandle(uint32_t handle)
{
    textureHandle_ = handle;
}

void Grid::Draw()
{
#ifdef _DEBUG
    materialHandle_.materialData->isArtGrid = true;

    transform_.UpdateMatrix();

 /*   engine_->renderer_->DrawGrid(
        transform_,
        *camera_,
        *modelData_,
        textureHandle_,
        color_,
        materialHandle_
    );*/
#endif
}