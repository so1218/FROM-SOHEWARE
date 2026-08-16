#include "pch.h"
#include "FoliageField.h"
#include "ImGuiManager.h"

FoliageField::FoliageField(FE::Engine* engine) : FE::GameObject(), engine_(engine)
{
    foliageSystem_ = std::make_unique<FE::FoliageSystem>(engine_);
    binder_ = std::make_unique<FE::PropertyBinder>(engine_, "FoliageField");
}

void FoliageField::Initialize()
{
    AddFoliageLayer("Flower_01", "flower_01");
    AddFoliageLayer("Foliage_01", "foliage_01");

    for (size_t i = 0; i < layers_.size(); ++i) {
        SetupBinderForLayer(i);
    }

    // 1. 全体共通パラメータのバインド
    binder_->Bind("Position", &transform_.translation_, { 0.0f, 0.0f, 0.0f });
    binder_->Bind("TerrainWidth", &terrainWidth_, 1024.0f, 1.0f, 10.0f, 5000.0f);
    binder_->Bind("TerrainDepth", &terrainDepth_, 1024.0f, 1.0f, 10.0f, 5000.0f);

    // カリング設定のバインド
    binder_->Bind("MaxDrawDistance", &cullingData_.maxDrawDistance, 300.0f, 1.0f, 10.0f, 2000.0f);
    binder_->Bind("ThinStartDistance", &cullingData_.thinStartDistance, 150.0f, 1.0f, 10.0f, 2000.0f);
    binder_->Bind("MaxThinningRate", &cullingData_.maxThinningRate, 0.95f, 0.01f, 0.0f, 1.0f);

    auto onResourceChanged = [this]() { ReloadResources(); };
    binder_->BindTexture("HeightMap", &heightMapName_, &heightMapHandle_, "noise_39", FE::TextureType::Noise, onResourceChanged);

    // ★ 追加: 複数レイヤーの中で最大のAABBサイズを求めるための変数
    float maxRadius = 0.0f;
    float maxYOffset = 0.0f;

    // 初回構築時のみ AddFoliageType と InitializeRenderer を呼ぶ
    for (const auto& layer : layers_)
    {
        const FE::ModelData* modelData = FE::ModelManager::GetInstance().Get(layer.modelName);
        if (modelData && !modelData->meshes.empty())
        {
            const auto& meshPart = modelData->meshes[0];

            // ==========================================
            // ★ 追加: メッシュのAABBからカリングサイズを計算
            // ==========================================
            FE::Vector3 minPos = meshPart.localAABB.min;
            FE::Vector3 maxPos = meshPart.localAABB.max;

            float currentYOffset = (minPos.y + maxPos.y) * 0.5f;
            FE::Vector3 size = { maxPos.x - minPos.x, maxPos.y - minPos.y, maxPos.z - minPos.z };
            float currentRadius = std::sqrt(size.x * size.x + size.y * size.y + size.z * size.z) * 0.5f;

            // 複数のレイヤーがある場合は一番大きいモデルを基準にする
            if (currentRadius > maxRadius)
            {
                maxRadius = currentRadius;
                maxYOffset = currentYOffset;
            }

            foliageSystem_->AddFoliageType(
                layer.albedoName, layer.normalName, layer.densityMapName,
                meshPart, layer.material, layer.genData
            );
        }
    }

    // ★ 追加: 計算したカリング用サイズを設定 (小さすぎる場合は最低1.0fを担保)
    cullingData_.modelRadius = std::max(maxRadius, 1.0f);
    cullingData_.modelCenterYOffset = maxYOffset;

    foliageSystem_->InitializeRenderer();

    GenerateFoliage();
}

void FoliageField::AddFoliageLayer(const std::string& layerName, const std::string& modelName)
{
    FE::FoliageLayer layer;
    layer.name = layerName;
    layer.modelName = modelName;

    // 初期値設定
    layer.genData.maxInstancesPerChunk = 50000;
    layer.genData.gridSpacing = 0.5f;
    layer.genData.minScale = 0.8f;
    layer.genData.maxScale = 1.2f;

    layer.material.roughness = 0.8f;
    layer.material.alphaCutoff = 0.5f;
    layer.material.windResponse = 1.0f;
    layer.material.shadowDensity = 1.0f;

    layers_.push_back(layer);
}

void FoliageField::SetupBinderForLayer(size_t index)
{
    auto& layer = layers_[index];
    std::string prefix = layer.name + "_"; // "Grass_" などの接頭辞を作る

    // パラメータが変更されたら再生成・再ロードするコールバック
    auto onGenChanged = [this]() { GenerateFoliage(); };
    auto onResChanged = [this]() { ReloadResources(); };

    // 生成パラメータ
    binder_->Bind(prefix + "MaxInstances", &layer.uiMaxInstances, 50000, 1000, 1000, 500000);
    binder_->Bind(prefix + "GridSpacing", &layer.genData.gridSpacing, 0.5f, 0.01f, 0.05f, 10.0f);
    binder_->Bind(prefix + "MinScale", &layer.genData.minScale, 0.8f, 0.01f, 0.1f, 5.0f);
    binder_->Bind(prefix + "MaxScale", &layer.genData.maxScale, 1.2f, 0.01f, 0.1f, 5.0f);

    // マテリアル
    binder_->Bind(prefix + "Roughness", &layer.material.roughness, 0.8f, 0.01f, 0.0f, 1.0f);
    binder_->Bind(prefix + "AlphaCutoff", &layer.material.alphaCutoff, 0.5f, 0.01f, 0.0f, 1.0f);
    binder_->Bind(prefix + "WindResponse", &layer.material.windResponse, 1.0f, 0.01f, 0.0f, 5.0f);
    binder_->Bind(prefix + "Stiffness", &layer.material.stiffness, 1.0f, 0.01f, 0.0f, 10.0f);

    binder_->Bind(prefix + "SSSStrength", &layer.material.sssStrength, 0.5f, 0.01f, 0.0f, 1.0f);
    binder_->Bind(prefix + "FlutterSpeed", &layer.material.flutterSpeed, 2.0f, 0.1f, 0.0f, 10.0f);
    binder_->Bind(prefix + "FlutterScale", &layer.material.flutterScale, 0.1f, 0.01f, 0.0f, 1.0f);
    binder_->Bind(prefix + "PlantHeight", &layer.material.plantHeight, 1.0f, 0.1f, 0.1f, 10.0f);

    // シャドウ設定も調整できると便利
    binder_->Bind(prefix + "ShadowDensity", &layer.material.shadowDensity, 1.0f, 0.01f, 0.0f, 1.0f);

    layer.material.shadowNormalBias = 0.005f;
    layer.material.shadowBias = 0.001f;

    // テクスチャ
    binder_->BindColor(prefix + "BaseColor", &layer.material.baseColor, { 1.0f, 1.0f, 1.0f });
    binder_->BindTexture(prefix + "DensityMap", &layer.densityMapName, &layer.densityMapHandle, "white1x1", FE::TextureType::Noise, onGenChanged);
    binder_->BindTexture(prefix + "Albedo", &layer.albedoName, &layer.albedoHandle, "white1x1", FE::TextureType::Albedo, onResChanged);
    binder_->BindTexture(prefix + "Normal", &layer.normalName, &layer.normalHandle, "white1x1", FE::TextureType::Normal, onResChanged);
}

void FoliageField::ReloadResources()
{
    foliageSystem_->UpdateConfigs(layers_);
}

void FoliageField::GenerateFoliage()
{
    for (auto& layer : layers_) {
        layer.genData.terrainCenter = { transform_.translation_.x, transform_.translation_.z };
        layer.genData.terrainWidth = terrainWidth_;
        layer.genData.terrainDepth = terrainDepth_;

        // =======================================================
        // ★ 追加: 地形の面積から必要な最大インスタンス数を計算して上限を設ける
        // =======================================================
        uint32_t gridX = static_cast<uint32_t>(std::ceil(terrainWidth_ / layer.genData.gridSpacing));
        uint32_t gridZ = static_cast<uint32_t>(std::ceil(terrainDepth_ / layer.genData.gridSpacing));
        uint32_t neededFoliageCount = gridX * gridZ;

        // UI設定値(maxInstancesPerChunk)と必要数のうち、小さい方を採用
        layer.genData.maxInstancesPerChunk = std::min(
            static_cast<uint32_t>(layer.uiMaxInstances),
            neededFoliageCount
        );
    }

    // Systemのポインタ経由で最新データを送る
    foliageSystem_->UpdateConfigs(layers_);

    foliageSystem_->Generate(heightMapName_, static_cast<UINT>(terrainWidth_), static_cast<UINT>(terrainDepth_));
}

void FoliageField::Update()
{
    bool needsGenerate = false;

    // パラメータ変更の監視
    if (transform_.translation_.x != prevPosition_.x ||
        transform_.translation_.y != prevPosition_.y ||
        transform_.translation_.z != prevPosition_.z ||
        terrainWidth_ != prevTerrainWidth_ ||
        terrainDepth_ != prevTerrainDepth_ ||
        heightMapName_ != prevHeightMapName_)
    {
        needsGenerate = true;
        prevPosition_ = transform_.translation_;
        prevTerrainWidth_ = terrainWidth_;
        prevTerrainDepth_ = terrainDepth_;
        prevHeightMapName_ = heightMapName_;
    }
    // 2. ★ 追加: 各レイヤーの生成パラメータ変更監視
    for (auto& layer : layers_)
    {
        if (layer.uiMaxInstances != layer.prevMaxInstances ||
            layer.genData.gridSpacing != layer.prevGridSpacing ||
            layer.genData.minScale != layer.prevMinScale ||
            layer.genData.maxScale != layer.prevMaxScale)
        {
            needsGenerate = true;

            // キャッシュを更新
            layer.prevMaxInstances = layer.uiMaxInstances;
            layer.prevGridSpacing = layer.genData.gridSpacing;
            layer.prevMinScale = layer.genData.minScale;
            layer.prevMaxScale = layer.genData.maxScale;
        }
    }

    // 変更があれば自動的に再生成 (配置の更新)
    if (needsGenerate)
    {
        GenerateFoliage();
    }

    // ==========================================
    // 3. ★ 修正: マテリアル等の変更をGPUに反映
    // ==========================================
    for (size_t i = 0; i < layers_.size(); ++i) {
        auto* matPtr = foliageSystem_->GetMaterialData(i);
        if (matPtr) *matPtr = layers_[i].material;
    }

    auto* cullPtr = foliageSystem_->GetCullingData();
    if (cullPtr) *cullPtr = cullingData_;

    // ★ これを毎フレーム呼ぶことで、風の強さやラフネスのUI変更がリアルタイムに反映されます
    foliageSystem_->UpdateConfigs(layers_);

    // カリングデータの転送など
    foliageSystem_->Update();
}

void FoliageField::Draw() {}

void FoliageField::DebugDraw()
{
#ifdef IS_DEVELOPMENT
    ImGui::Begin("植物プロシージャル");

    if (ImGui::CollapsingHeader("全体設定 (地形・カリング)", ImGuiTreeNodeFlags_DefaultOpen))
    {
        binder_->Draw("Position", "中心座標");
        binder_->Draw("TerrainWidth", "地形幅 (Width)");
        binder_->Draw("TerrainDepth", "地形奥行き (Depth)");

        binder_->Draw("HeightMap", "ハイトマップ");

        ImGui::Separator();
        binder_->Draw("MaxDrawDistance", "描画限界距離");
        binder_->Draw("ThinStartDistance", "間引き開始距離");
        binder_->Draw("MaxThinningRate", "最大間引き率");

        if (ImGui::Button("強制再生成 (Generate)")) {
            GenerateFoliage();
        }
    }

    // 各レイヤーごとのUIを生成
    for (size_t i = 0; i < layers_.size(); ++i)
    {
        auto& layer = layers_[i];
        std::string prefix = layer.name + "_";

        ImGui::PushID(static_cast<int>(i)); // ID被り防止
        if (ImGui::CollapsingHeader((layer.name + " の設定").c_str()))
        {
            ImGui::Text("【 配置・スケール 】");
            binder_->Draw(prefix + "MaxInstances", "最大生成数");
            binder_->Draw(prefix + "GridSpacing", "配置間隔 (密度)");
            binder_->Draw(prefix + "MinScale", "最小スケール");
            binder_->Draw(prefix + "MaxScale", "最大スケール");

            ImGui::Separator();
            ImGui::Text("【 テクスチャ・マテリアル 】");
            binder_->Draw(prefix + "Albedo", "アルベド");
            binder_->Draw(prefix + "Normal", "ノーマル");
            binder_->Draw(prefix + "DensityMap", "密度マップ (Density)"); 

            binder_->Draw(prefix + "BaseColor", "基本色");

            binder_->Draw(prefix + "Roughness", "粗さ");
            binder_->Draw(prefix + "AlphaCutoff", "抜き透過 (Alpha Cutoff)");

            binder_->Draw(prefix + "SSSStrength", "透過光 (SSS Strength)");

            ImGui::Text("【 風の揺れ・シャドウ 】");
            binder_->Draw(prefix + "WindResponse", "風の影響度");
            binder_->Draw(prefix + "Stiffness", "硬さ (揺れにくさ)");
            binder_->Draw(prefix + "FlutterSpeed", "細かな揺れの速度");
            binder_->Draw(prefix + "FlutterScale", "細かな揺れの幅");
            binder_->Draw(prefix + "PlantHeight", "植物の高さ基準");
            binder_->Draw(prefix + "ShadowDensity", "影の濃さ");
        }
        ImGui::PopID();
    }

    ImGui::End();
#endif
}