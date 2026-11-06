#include "pch.h"
#include "Ground.h"
#include "ImGuiManager.h"

using namespace FS;

Ground::Ground(Engine* engine) : GameObject()
{
	engine_ = engine;

	// 保存したハイトマップがあれば、Initialize で読み直される
	terrain_ = std::make_unique<FS::Terrain>(engine_);
	terrain_->LoadFromHeightmap("noise_39", 1.0f);

	binder_ = std::make_unique<PropertyBinder>(engine_, "Ground");
	interactionSystem_ = std::make_unique<WorldInteractionSystem>(engine_);
}

void Ground::Initialize()
{
	binder_->BindTerrain("Terrain", terrain_.get());

	interactionSystem_->Initialize();
	interactionSystem_->SetTerrain(terrain_.get());
}

void Ground::Update()
{
	interactionSystem_->Update();
}

void Ground::Draw()
{
	terrain_->Draw();
	interactionSystem_->Draw();
}

void Ground::DebugGui()
{
	ImGui::Begin("地面");
	binder_->DrawTerrain("Terrain", "地形エディタ");
	ImGui::End();

	interactionSystem_->DebugGui();
}
