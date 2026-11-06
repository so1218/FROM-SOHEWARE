#include "pch.h"
#include "FoliageField.h"
#include "ImGuiManager.h"

FoliageField::FoliageField(FS::Engine* engine) : FS::GameObject(), engine_(engine)
{
    foliageSystem_ = std::make_unique<FS::FoliageSystem>(engine_);
    binder_ = std::make_unique<FS::PropertyBinder>(engine_, "FoliageField");
}

void FoliageField::Initialize()
{
    layers_.clear();
    if (foliageSystem_)
    {
        foliageSystem_->Reset();
    }

    AddFoliageLayer("Flower_01", "flower_01");
    AddFoliageLayer("Foliage_01", "foliage_01");
    AddFoliageLayer("Foliage_02", "foliage_02");

    for (size_t i = 0; i < layers_.size(); ++i) {
        SetupBinderForLayer(i);
    }

    binder_->Bind("MaxDrawDistance", &cullingData_.maxDrawDistance, 300.0f, 1.0f, 10.0f, 2000.0f);
    binder_->Bind("ThinStartDistance", &cullingData_.thinStartDistance, 150.0f, 1.0f, 10.0f, 2000.0f);
    binder_->Bind("MaxThinningRate", &cullingData_.maxThinningRate, 0.95f, 0.01f, 0.0f, 1.0f);

    float maxRadius = 0.0f;
    float maxYOffset = 0.0f;

    // 初回構築時のみ AddFoliageType と InitializeRenderer を呼ぶ
    for (const auto& layer : layers_)
    {
        const FS::ModelData* modelData = FS::ModelManager::GetInstance().Get(layer.modelName);
        if (modelData && !modelData->meshes.empty())
        {
            const auto& meshPart = modelData->meshes[0];

            // ==========================================
            // メッシュのAABBからカリングサイズを計算
            // ==========================================
            FS::Vector3 minPos = meshPart.localAABB.min;
            FS::Vector3 maxPos = meshPart.localAABB.max;

            float currentYOffset = (minPos.y + maxPos.y) * 0.5f;
            FS::Vector3 size = { maxPos.x - minPos.x, maxPos.y - minPos.y, maxPos.z - minPos.z };
            float currentRadius = std::sqrt(size.x * size.x + size.y * size.y + size.z * size.z) * 0.5f;

            // 複数のレイヤーがある場合は一番大きいモデルを基準
            if (currentRadius > maxRadius)
            {
                maxRadius = currentRadius;
                maxYOffset = currentYOffset;
            }

            foliageSystem_->AddFoliageType(
                layer.albedoName, layer.densityMapName,
                meshPart, layer.material, layer.genData
            );
        }
    }

    // 計算したカリング用サイズを設定 (小さすぎる場合は最低1.0f)
    cullingData_.modelRadius = std::max(maxRadius, 1.0f);
    cullingData_.modelCenterYOffset = maxYOffset;

    foliageSystem_->InitializeRenderer();
    GenerateFoliage();
}

void FoliageField::AddFoliageLayer(const std::string& layerName, const std::string& modelName)
{
    FS::FoliageLayer layer;
    layer.name = layerName;
    layer.modelName = modelName;

    // 初期値設定
    layer.genData.maxInstanceCount = 50000;
    layer.genData.gridSpacing = 0.5f;
    layer.genData.minScale = 0.8f;
    layer.genData.maxScale = 1.2f;

    layer.material.alphaCutoff = 0.5f;
    layer.material.windResponse = 1.0f;
    layer.material.shadowDensity = 1.0f;

    layer.material.interactStrength = 1.0f;
    layer.material.trailFlattenWeight = 0.8f;

    layers_.push_back(layer);
}

void FoliageField::SetupBinderForLayer(size_t index)
{
    auto& layer = layers_[index];
    std::string prefix = layer.name + "_";

    binder_->Bind(prefix + "MaxInstances", &layer.uiMaxInstances, 50000, 1000, 1000, 500000);
    binder_->Bind(prefix + "GridSpacing", &layer.genData.gridSpacing, 0.5f, 0.01f, 0.05f, 10.0f);
    binder_->Bind(prefix + "MinScale", &layer.genData.minScale, 0.8f, 0.01f, 0.1f, 5.0f);
    binder_->Bind(prefix + "MaxScale", &layer.genData.maxScale, 1.2f, 0.01f, 0.1f, 5.0f);
    binder_->Bind(prefix + "ColorIntensity", &layer.material.colorIntensity, 1.0f, 0.05f, 0.1f, 5.0f);
    binder_->Bind(prefix + "AlphaCutoff", &layer.material.alphaCutoff, 0.5f, 0.01f, 0.0f, 1.0f);
    binder_->Bind(prefix + "WindResponse", &layer.material.windResponse, 1.0f, 0.01f, 0.0f, 5.0f);
   
    binder_->Bind(prefix + "FlutterSpeed", &layer.material.flutterSpeed, 2.0f, 0.1f, 0.0f, 10.0f);
    binder_->Bind(prefix + "FlutterScale", &layer.material.flutterScale, 0.1f, 0.01f, 0.0f, 1.0f);
    binder_->Bind(prefix + "PlantHeight", &layer.material.plantHeight, 1.0f, 0.1f, 0.1f, 10.0f);

    binder_->Bind(prefix + "ShadowDensity", &layer.material.shadowDensity, 1.0f, 0.01f, 0.0f, 1.0f);

    binder_->Bind(prefix + "InteractStrength", &layer.material.interactStrength, 1.0f, 0.01f, 0.0f, 5.0f);
    binder_->Bind(prefix + "TrailFlattenWeight", &layer.material.trailFlattenWeight, 0.8f, 0.01f, 0.0f, 1.0f);

    layer.material.shadowNormalBias = 0.005f;
    layer.material.shadowBias = 0.001f;

    binder_->BindColor(prefix + "BaseColor", &layer.material.baseColor, { 1.0f, 1.0f, 1.0f });
    binder_->BindTexture(prefix + "DensityMap", &layer.densityMapName, &layer.densityMapHandle, "white1x1", FS::TextureType::Noise);
    binder_->BindTexture(prefix + "Albedo", &layer.albedoName, &layer.albedoHandle, "white1x1", FS::TextureType::Albedo);
}

void FoliageField::GenerateFoliage()
{
    needsRegenerate_ = false;
    generatedTerrainVersion_ = terrain_->GetShapeVersion();

    const FS::Vector2 areaSize = terrain_->GetSizeXZ();
    for (auto& layer : layers_) {
        layer.genData.terrainCenter = terrain_->GetCenterXZ();
        layer.genData.terrainWidth = areaSize.x;
        layer.genData.terrainDepth = areaSize.y;

        // 地形全体を敷き詰めるのに必要な数
        uint32_t gridX = static_cast<uint32_t>(std::ceil(areaSize.x / layer.genData.gridSpacing));
        uint32_t gridZ = static_cast<uint32_t>(std::ceil(areaSize.y / layer.genData.gridSpacing));
        uint32_t neededFoliageCount = gridX * gridZ;

        // 設定した上限と必要数のうち、小さい方を採用
        layer.genData.maxInstanceCount = std::min(
            static_cast<uint32_t>(layer.uiMaxInstances),
            neededFoliageCount
        );
    }

    foliageSystem_->UpdateConfigs(layers_);
    foliageSystem_->Generate(terrain_->GetHeightmapName());
}

void FoliageField::Update()
{
    // 設定か地形が変わったときだけ作り直す
    if (needsRegenerate_ || terrain_->GetShapeVersion() != generatedTerrainVersion_)
    {
        GenerateFoliage();
    }

    // 色や風などはエディターですぐ確かめられるように、毎フレーム送る
    for (size_t i = 0; i < layers_.size(); ++i) {
        auto* matPtr = foliageSystem_->GetMaterialData(i);
        if (matPtr) *matPtr = layers_[i].material;
    }

    auto* cullPtr = foliageSystem_->GetCullingData();
    if (cullPtr) *cullPtr = cullingData_;

    foliageSystem_->UpdateConfigs(layers_);
    foliageSystem_->Update();
}

void FoliageField::Draw() {}

void FoliageField::DebugGui()
{
    ImGui::Begin("植物プロシージャル");

    // 生える範囲とハイトマップは地形から取る
    if (ImGui::CollapsingHeader("全体設定 (カリング)", ImGuiTreeNodeFlags_DefaultOpen))
    {
        binder_->Draw("MaxDrawDistance", "描画限界距離");
        binder_->Draw("ThinStartDistance", "間引き開始距離");
        binder_->Draw("MaxThinningRate", "最大間引き率");

        if (ImGui::Button("強制再生成"))
        {
            needsRegenerate_ = true;
        }
    }

    for (size_t i = 0; i < layers_.size(); ++i)
    {
        auto& layer = layers_[i];
        std::string prefix = layer.name + "_";

        ImGui::PushID(static_cast<int>(i)); 
        if (ImGui::CollapsingHeader((layer.name + " の設定").c_str()))
        {
            ImGui::Text("配置・スケール");
            needsRegenerate_ |= binder_->Draw(prefix + "MaxInstances", "最大生成数");
            needsRegenerate_ |= binder_->Draw(prefix + "GridSpacing", "配置間隔");
            needsRegenerate_ |= binder_->Draw(prefix + "MinScale", "最小スケール");
            needsRegenerate_ |= binder_->Draw(prefix + "MaxScale", "最大スケール");

            ImGui::Separator();
            ImGui::Text("テクスチャ・マテリアル");
            binder_->Draw(prefix + "Albedo", "アルベド");
            needsRegenerate_ |= binder_->Draw(prefix + "DensityMap", "密度マップ");

            binder_->Draw(prefix + "BaseColor", "基本色");
            binder_->Draw(prefix + "ColorIntensity", "明るさ倍率");

            binder_->Draw(prefix + "AlphaCutoff", "抜き透過 ");

            ImGui::Text("風の揺れ・シャドウ");
            binder_->Draw(prefix + "WindResponse", "風の影響度");
            binder_->Draw(prefix + "FlutterSpeed", "細かな揺れの速度");
            binder_->Draw(prefix + "FlutterScale", "細かな揺れの幅");
            binder_->Draw(prefix + "PlantHeight", "植物の高さ基準");
            binder_->Draw(prefix + "ShadowDensity", "影の濃さ");

            ImGui::Separator();
            ImGui::Text("インタラクション");
            binder_->Draw(prefix + "InteractStrength", "物理干渉強度");
            binder_->Draw(prefix + "TrailFlattenWeight", "軌跡の押しつぶし影響度");
        }
        ImGui::PopID();
    }

    ImGui::End();
}