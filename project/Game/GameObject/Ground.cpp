#include "pch.h"
#include "Ground.h"
#include "ImGuiManager.h"

using namespace FE;

Ground::Ground(Engine* engine) : GameObject()
{
	engine_ = engine;

	model_ = std::make_unique<Model>(engine_, "field");
	modelTree_ = std::make_unique<Model>(engine_, "tree");
	modelRock_ = std::make_unique<Model>(engine_, "volumetricFog");
	skybox_ = std::make_unique<Skybox>(engine);

	binder_ = std::make_unique<PropertyBinder>(engine_, "Ground");

	auto* leafMat = modelTree_->GetMaterialData();
	auto* leafMat2 = modelTree_->GetMaterialData(1);

	leafMat->enableTreeWind = true;

	binder_->Bind("treeWindSpeed", &leafMat->treeWindSpeed, 0.05f);
	binder_->Bind("treeWindAmplitude", &leafMat->treeWindAmplitude, 0.01f);
	binder_->Bind("treeWindSpatialScale", &leafMat->treeWindSpatialScale, 0.01f);
	binder_->Bind("treeWindHeightScale", &leafMat->treeWindHeightScale, 0.01f);
	binder_->Bind("treeWindVariation", &leafMat->treeWindVariation, 0.01f);
	leafMat2->enableTreeWind = leafMat->enableTreeWind;

}

void Ground::Initialize()
{
	binder_->BindModel("Model", model_.get());
	binder_->BindModel("ModelTree", modelTree_.get());
	binder_->BindModel("ModelRock", modelRock_.get());
	skybox_->SetCubeTexture("redClunch");
	modelTree_->ApplyRenderSettings(RenderingPreset::StandardNoCull);

	std::mt19937 randomEngine(1234); 
	std::uniform_real_distribution<float> distPos(-10000.0f, 10000.0f);

	for (int i = 0; i < 1000; ++i)
	{
		Vector3 pos;
		pos.x = float(i * 5);
		pos.y = 0.0f; // 地面の高さに合わせる
		pos.z = float(i * 5);
		treePositions_.push_back(pos);
	}
};

void Ground::Update()
{
	auto* leafMat = modelTree_->GetMaterialData();
	auto* leafMat2 = modelTree_->GetMaterialData(1);
	leafMat2->treeWindSpeed = leafMat->treeWindSpeed;
	leafMat2->treeWindAmplitude = leafMat->treeWindAmplitude;
	leafMat2->treeWindSpatialScale = leafMat->treeWindSpatialScale;
	leafMat2->treeWindHeightScale = leafMat->treeWindHeightScale;
	leafMat2->treeWindVariation = leafMat->treeWindVariation;
};

void Ground::Draw()
{
	model_->Draw();
	/*for (const auto& pos : treePositions_)
	{
		modelTree_->GetTransform().translation_ = pos;

		modelTree_->Draw(); 
	}*/
	modelRock_->Draw();
	skybox_->Draw();
};

void Ground::DebugDraw()
{
#ifdef IS_DEVELOPMENT
	ImGui::Begin("地面");
	binder_->DrawModel("Model", "インスペクター");
	binder_->DrawModel("ModelTree", "木インスペクター");
	binder_->DrawModel("ModelRock", "岩インスペクター");

	ImGui::Separator();
	ImGui::Text("木の揺れ（葉っぱ）");

	binder_->Draw("treeWindSpeed", "風の速さ");
	binder_->Draw("treeWindAmplitude", "揺れの強さ");
	binder_->Draw("treeWindSpatialScale", "位置によるズレ");
	binder_->Draw("treeWindHeightScale", "高さの影響度");
	binder_->Draw("treeWindVariation", "揺れの複雑さ");
	ImGui::End();
#endif
}