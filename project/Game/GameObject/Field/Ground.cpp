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
	BindTerrain();

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
	DrawTerrainEditor();
	ImGui::End();

	interactionSystem_->DebugGui();
}

void Ground::BindTerrain()
{
	Terrain* terrain = terrain_.get();
	auto onShapeChanged = [terrain]() { terrain->MarkShapeChanged(); };

	binder_->Bind("Terrain_Position", &terrain->GetPosition(), { 0.0f, 0.0f, 0.0f }, 1.0f, onShapeChanged);
	binder_->BindTexture("Terrain_Heightmap", terrain->GetHeightmapNamePtr(), terrain->GetHeightmapHandlePtr(), "white1x1", TextureType::Noise,
		[terrain]()
		{
			terrain->LoadFromHeightmap(terrain->GetHeightmapName(), terrain->GetParams().cellSize);
		});
	binder_->Bind("Terrain_MaxHeight", &terrain->GetParams().maxHeight, 20.0f, 0.1f, 0.0f, 500.0f, onShapeChanged);

	TerrainMaterial& material = terrain->GetMaterial();
	TerrainMaterialData* data = material.data;

	binder_->BindTexture("Terrain_AlbedoMap", &material.albedoMapName, &material.albedoMapHandle, "white1x1", TextureType::Albedo);
	binder_->BindColor("Terrain_Color", &data->color, { 1.0f, 1.0f, 1.0f, 1.0f });
	binder_->Bind("Terrain_Roughness", &data->roughness, 1.0f, 0.01f, 0.0f, 1.0f);
	binder_->Bind("Terrain_Metalness", &data->metalness, 0.0f, 0.01f, 0.0f, 1.0f);
	binder_->BindTexture("Terrain_EnvMapTex", &material.environmentMapName, &material.environmentMapHandle, "pureSky", TextureType::CubeMap);
	binder_->Bind("Terrain_EnvMapInt", &data->environmentMapIntensity, 0.5f, 0.01f, 0.0f, 10.0f);
	binder_->Bind("Terrain_Emissive", &data->emissiveIntensity, 1.0f, 0.1f, 0.0f, 100.0f);

	binder_->BindBool("Terrain_AddShadow", &data->addShadow, true);
	binder_->Bind("Terrain_ShadowDens", &data->shadowDensity, 0.8f, 0.01f, 0.0f, 0.99f);
	binder_->Bind("Terrain_ShadowEnv", &data->shadowEnvStrength, 0.0f, 0.01f, 0.0f, 1.0f);
	binder_->Bind("Terrain_ShadowBias", &data->shadowBias, 0.0005f, 0.0001f, 0.0f, 0.1f);
	binder_->Bind("Terrain_ShadowNormalBias", &data->shadowNormalBias, 0.0f, 0.001f, 0.0f, 1.0f);
	binder_->Bind("Terrain_ShadowSoft", &data->shadowSoftness, 0.0f, 0.01f, 0.0f, 5.0f);

	binder_->BindBool("Terrain_NormEnable", &data->enableNormalMap, false);
	binder_->BindTexture("Terrain_NormalMapTex", &material.normalMapName, &material.normalMapHandle, "normal_01", TextureType::Normal);
	binder_->Bind("Terrain_NormTile", &data->normalTiling, 1.0f, 0.1f, 0.1f, 100.0f);
	binder_->Bind("Terrain_NormInten", &data->normalIntensity, 1.0f, 0.01f, 0.0f, 10.0f);

	binder_->Bind("Terrain_Wetness", &data->wetness, 0.0f, 0.01f, 0.0f, 1.0f);
	binder_->BindTexture("Terrain_RippleMap", &material.rippleMapName, &material.rippleMapHandle, "normal_31", TextureType::Normal);
	binder_->Bind("Terrain_RippleScale", &data->rippleScale, 0.4f, 0.01f, 0.01f, 50.0f);
	binder_->Bind("Terrain_RippleStren", &data->rippleStrength, 10.0f, 0.1f, 0.0f, 50.0f);
	binder_->Bind("Terrain_RippleSize", &data->rippleSize, 0.0f, 0.01f, 0.0f, 10.0f);
	binder_->Bind("Terrain_RippleSpeed", &data->rippleSpeed, 0.4f, 0.01f, 0.0f, 20.0f);
	binder_->Bind("Terrain_RippleFreq", &data->rippleFrequency, 6.0f, 0.1f, 0.01f, 20.0f);
}

void Ground::DrawTerrainEditor()
{
	const TerrainMaterialData* data = terrain_->GetMaterial().data;

	ImGui::SeparatorText("形状");
	binder_->Draw("Terrain_Position", "地形の位置");
	binder_->Draw("Terrain_Heightmap", "ハイトマップ画像");
	binder_->Draw("Terrain_MaxHeight", "地形の最大高さ");

	ImGui::SeparatorText("質感");
	binder_->Draw("Terrain_AlbedoMap", "メインテクスチャ");
	binder_->Draw("Terrain_Color", "カラー");
	binder_->Draw("Terrain_Roughness", "粗さ");
	binder_->Draw("Terrain_Metalness", "金属度");
	binder_->Draw("Terrain_EnvMapTex", "環境マップ");
	binder_->Draw("Terrain_EnvMapInt", "環境マップ強度");
	binder_->Draw("Terrain_Emissive", "自己発光強度");

	ImGui::SeparatorText("影");
	binder_->Draw("Terrain_AddShadow", "影を受ける");
	if (data->addShadow != 0)
	{
		binder_->Draw("Terrain_ShadowDens", "影の濃さ");
		binder_->Draw("Terrain_ShadowEnv", "環境光の影の強さ");
		binder_->Draw("Terrain_ShadowBias", "バイアス");
		binder_->Draw("Terrain_ShadowNormalBias", "法線方向のバイアス");
		binder_->Draw("Terrain_ShadowSoft", "柔らかさ");
	}

	ImGui::SeparatorText("法線マップ");
	binder_->Draw("Terrain_NormEnable", "有効化");
	if (data->enableNormalMap != 0)
	{
		binder_->Draw("Terrain_NormalMapTex", "テクスチャ");
		binder_->Draw("Terrain_NormTile", "タイリング");
		binder_->Draw("Terrain_NormInten", "凹凸の強さ");
	}

	ImGui::SeparatorText("濡れ・雨の波紋");
	binder_->Draw("Terrain_Wetness", "濡れ具合");
	binder_->Draw("Terrain_RippleMap", "波紋の法線マップ");
	binder_->Draw("Terrain_RippleScale", "密度");
	binder_->Draw("Terrain_RippleStren", "強さ");
	binder_->Draw("Terrain_RippleSize", "広がる大きさ");
	binder_->Draw("Terrain_RippleSpeed", "速さ");
	binder_->Draw("Terrain_RippleFreq", "発生頻度");
}
