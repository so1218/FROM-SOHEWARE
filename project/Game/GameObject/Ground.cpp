#include "pch.h"
#include "Ground.h"
#include "ImGuiManager.h"
#include "TerrainChunk.h"

using namespace FE;

Ground::Ground(Engine* engine) : GameObject()
{
	engine_ = engine;

	// Terrain の生成
	terrain_ = std::make_unique<FE::Terrain>(engine_);

	// 2. ハイトマップ画像の読み込みとメッシュ生成（ここで内部的にチャンク分割される）
	// 引数: テクスチャ名, 最大の高さ, 1チャンクのマス目数(例: 64), 1マスのサイズ(例: 1.0f)
	int chunkSize = 64;
	float cellSize = 1.0f;
	terrain_->LoadFromHeightmap("noise_39", chunkSize, cellSize);

	model_ = std::make_unique<Model>(engine_, "field");
	modelTree_ = std::make_unique<Model>(engine_, "tree");
	modelBuilding_ = std::make_unique<Model>(engine_, "volumetricFog");
	skybox_ = std::make_unique<Skybox>(engine);
	skydome_ = std::make_unique<Skydome>(engine);

	binder_ = std::make_unique<PropertyBinder>(engine_, "Ground");

	auto* leafMat = modelTree_->GetMaterialData();
	auto* leafMat2 = modelTree_->GetMaterialData(1);

	leafMat->enableTreeWind = true;

	binder_->Bind("treeWindSpeed", &leafMat->treeWindSpeed, 0.05f);
	binder_->Bind("treeWindAmplitude", &leafMat->treeWindAmplitude, 0.01f);
	binder_->Bind("treeWindSpatialScale", &leafMat->treeWindSpatialScale, 0.01f);
	binder_->Bind("treeWindHeightScale", &leafMat->treeWindHeightScale, 0.01f);
	binder_->Bind("treeWindVariation", &leafMat->treeWindVariation, 0.01f);
	binder_->Bind("treeWindThresholdHeight", &leafMat->treeWindThresholdHeight, 5.0f);

}

void Ground::Initialize()
{
	binder_->BindModel("Model", model_.get());
	binder_->BindTerrain("Terrain", terrain_.get());
	binder_->BindModel("ModelTree", modelTree_.get());
	binder_->BindModel("ModelBuilding", modelBuilding_.get());

	binder_->Bind("TreeCount", &treeCount_, 100);
	binder_->Bind("TreeSpreadRadius", &treeSpreadRadius_, 50.0f);
	binder_->Bind("TreeBaseScale", &treeBaseScale_, 1.0f);

	skybox_->SetCubeTexture("skybox");
	skydome_->Initialize();
	skydome_->SetSkyCubeTexture("skybox");
	modelTree_->ApplyRenderSettings(RenderingPreset::StandardNoCull);

	GenerateTrees();
};

void Ground::GenerateTrees()
{
	treePositions_.clear(); 

	std::mt19937 randomEngine(std::random_device{}());
	std::uniform_real_distribution<float> distPos(-treeSpreadRadius_, treeSpreadRadius_);

	for (int i = 0; i < treeCount_; ++i)
	{
		Vector3 pos;
		pos.x = distPos(randomEngine);
		if (terrain_) {
			pos.y = terrain_->GetHeight(pos.x, pos.z);
		}
		else {
			pos.y = 0.0f;
		}
		pos.z = distPos(randomEngine);
		treePositions_.push_back(pos);
	}
}

void Ground::Update()
{
	if (treeCount_ != prevTreeCount_ ||
		treeSpreadRadius_ != prevTreeSpreadRadius_ ||
		treeBaseScale_ != prevTreeBaseScale_)
	{
		GenerateTrees();

		prevTreeCount_ = treeCount_;
		prevTreeSpreadRadius_ = treeSpreadRadius_;
		prevTreeBaseScale_ = treeBaseScale_;
	}

	auto* leafMat = modelTree_->GetMaterialData();
	auto* leafMat2 = modelTree_->GetMaterialData(1);
	leafMat2->treeWindSpeed = leafMat->treeWindSpeed;
	leafMat2->treeWindAmplitude = leafMat->treeWindAmplitude;
	leafMat2->treeWindSpatialScale = leafMat->treeWindSpatialScale;
	leafMat2->treeWindHeightScale = leafMat->treeWindHeightScale;
	leafMat2->treeWindVariation = leafMat->treeWindVariation;
	leafMat2->treeWindThresholdHeight = leafMat->treeWindThresholdHeight;
};

void Ground::Draw()
{
	if (terrain_) {
		terrain_->Draw();
	}

	/*model_->Draw();*/
	for (const auto& pos : treePositions_)
	{
		modelTree_->GetTransform().translation_ = pos;
		modelTree_->GetTransform().scale_ = { treeBaseScale_, treeBaseScale_, treeBaseScale_ };
		modelTree_->GetTransform().UpdateMatrix();
		modelTree_->Draw();
	}
	modelBuilding_->Draw();
	/*skybox_->Draw();*/
	/*skydome_->Draw();*/
};

void Ground::DebugDraw()
{
#ifdef IS_DEVELOPMENT
	ImGui::Begin("地面");
	binder_->DrawTerrain("Terrain", "地形エディタ");
	binder_->DrawModel("Model", "インスペクター");
	binder_->DrawModel("ModelBuilding", "建物インスペクター");

	binder_->DrawModel("ModelTree", "木インスペクター");
	binder_->Draw("TreeCount", "木の数");
	binder_->Draw("TreeSpreadRadius", "配置範囲");
	binder_->Draw("TreeBaseScale", "全体の大きさ");

	if (ImGui::Button("木をランダム再生成"))
	{
		GenerateTrees();
	}

	ImGui::Separator();
	ImGui::Text("木の揺れ（葉っぱ）");

	binder_->Draw("treeWindSpeed", "風の速さ");
	binder_->Draw("treeWindAmplitude", "揺れの強さ");
	binder_->Draw("treeWindSpatialScale", "位置によるズレ");
	binder_->Draw("treeWindHeightScale", "高さの影響度");
	binder_->Draw("treeWindVariation", "揺れの複雑さ");
	binder_->Draw("treeWindThresholdHeight", "揺れ始める高さ");
	ImGui::End();
#endif

	skydome_->DebugDraw();
}