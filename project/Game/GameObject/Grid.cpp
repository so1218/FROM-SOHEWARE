#include "pch.h"
#include "Grid.h"
#include "TextureManager.h"
#include "ModelManager.h"

using namespace FE;

Grid::Grid(Engine* engine)
    : GameObject(engine)
{
    SetTag("Grid");

    model_ = GameObject::CreateModel("field");
	model_->ApplyRenderSettings(RenderingPreset::Grid);
	model_->GetTransform().scale_ = { 10000.0f, 1.0f,10000.0f };
    model_->GetMaterialHandle()->materialData->isArtGrid = true;
	model_->SetRenderGroup(RenderGroup::Grid);
}

void Grid::Draw()
{
#ifdef IS_DEVELOPMENT

    model_->Draw();

#endif
}