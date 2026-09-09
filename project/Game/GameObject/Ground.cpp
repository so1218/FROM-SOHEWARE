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

	skydome_ = std::make_unique<Skydome>(engine);

	binder_ = std::make_unique<PropertyBinder>(engine_, "Ground");
	interactionSystem_ = std::make_unique<WorldInteractionSystem>(engine_);
}

void Ground::Initialize()
{
	binder_->BindTerrain("Terrain", terrain_.get());

	skydome_->Initialize();

	EnvironmentManager::GetInstance()->Initialize(engine_);

	interactionSystem_->Initialize();
	interactionSystem_->SetTerrain(terrain_.get());
};

void Ground::Update()
{
	EnvironmentManager::GetInstance()->Update(engine_->GetLightManager());

	skydome_->Update();
	interactionSystem_->Update();
};

void Ground::Draw()
{
	if (terrain_) {
		terrain_->Draw();
	}

	skydome_->Draw();
	interactionSystem_->Draw();
};

void Ground::DebugDraw()
{
#ifdef ENABLE_IMGUI
	ImGui::Begin("地面");

	binder_->DrawTerrain("Terrain", "地形エディタ");

	ImGui::End();
#endif

	EnvironmentManager::GetInstance()->DebugDraw();
	skydome_->DebugDraw();
	interactionSystem_->DebugDraw();
}