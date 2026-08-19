#include "pch.h"
#include "Grid.h"
#include "TextureManager.h"
#include "ModelManager.h"

using namespace FE;

Grid::Grid(Engine* engine)
    : GameObject()
{
    engine_ = engine;

    model_ = std::make_unique<Model>(engine_, "field");
	model_->ApplyRenderSettings(RenderingPreset::Grid);
	model_->GetTransform().scale_ = { 10000.0f, 1.0f,10000.0f };
    model_->GetMaterialHandle()->materialData->isArtGrid = true;
	model_->SetRenderGroup(RenderGroup::Grid);
}

void Grid::Draw()
{
#ifdef ENABLE_IMGUI

    model_->Draw();

#endif
}