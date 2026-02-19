#include "Grid.h"
#include "TextureManager.h"
#include "ModelManager.h"

Grid::Grid(Engine* engine)
    : GameObject(engine)
{
    SetTag("Grid");

    textureHandle_ = TextureManager::GetInstance().Get("white1x1");
    modelData_ = ModelManager::GetInstance().Get("field");
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

void Grid::SetTextureHandle(uint32_t handle)
{
    textureHandle_ = handle;
}

void Grid::Draw()
{
#ifdef IS_DEVELOPMENT
    materialHandle_.materialData->isArtGrid = true;

    transform_.UpdateMatrix();

    engine_->renderer_->SubmitGrid(
        transform_,
        *modelData_,
        textureHandle_,
        color_,
        materialHandle_
    );
#endif
}