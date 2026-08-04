#include "pch.h"
#include "Ground.h"
#include "ImGuiManager.h"
#include "TerrainChunk.h"
#include "EnvironmentManager.h"

using namespace FE;

Ground::Ground(Engine* engine) : GameObject()
{
	engine_ = engine;

	// Terrain の生成
	terrain_ = std::make_unique<FE::Terrain>(engine_);

	// ハイトマップ画像の読み込みとメッシュ生成
	float cellSize = 1.0f;
	terrain_->LoadFromHeightmap("noise_39", cellSize);

	model_ = std::make_unique<Model>(engine_, "field");
	modelBuilding_ = std::make_unique<Model>(engine_, "volumetricFog");
	skydome_ = std::make_unique<Skydome>(engine);

	binder_ = std::make_unique<PropertyBinder>(engine_, "Ground");

}

void Ground::Initialize()
{
	binder_->BindModel("Model", model_.get());
	binder_->BindTerrain("Terrain", terrain_.get());
	binder_->BindModel("ModelBuilding", modelBuilding_.get());

	skydome_->Initialize();

	EnvironmentManager::GetInstance()->Initialize(engine_);
};

void Ground::Update()
{
	EnvironmentManager::GetInstance()->Update(engine_->GetLightManager());

	skydome_->Update();
};

void Ground::Draw()
{
	if (terrain_) {
		terrain_->Draw();
	}

	modelBuilding_->Draw();
	skydome_->Draw();
};

void Ground::DebugDraw()
{
#ifdef IS_DEVELOPMENT
	ImGui::Begin("地面");
	binder_->DrawTerrain("Terrain", "地形エディタ");
	binder_->DrawModel("Model", "インスペクター");
	binder_->DrawModel("ModelBuilding", "建物インスペクター");

	ImGui::End();
#endif

	EnvironmentManager::GetInstance()->DebugDraw();
	skydome_->DebugDraw();
}