#include "pch.h"
#include "PebbleField.h"
#include "ImGuiManager.h"

using namespace FE;

PebbleField::PebbleField(Engine* engine) : GameObject(), engine_(engine)
{
    pebbleSystem_ = std::make_unique<PebbleSystem>(engine_);
    binder_ = std::make_unique<PropertyBinder>(engine_, "PebbleField");
}

void PebbleField::Initialize()
{
    auto* mat = pebbleSystem_->GetMaterialData();
    auto* cull = pebbleSystem_->GetCullingData();

    binder_->Bind("Position", &transform_.translation_, { 0.0f, 0.0f, 0.0f });
    binder_->Bind("MaxPebbles", &maxPebblesPerChunk_, 100000, 1000, 1000, 1000000);
    binder_->Bind("GridSpacing", &gridSpacing_, 1.0f, 0.01f, 0.1f, 10.0f);
    binder_->Bind("MinScale", &minScale_, 0.5f, 0.01f, 0.1f, 5.0f);
    binder_->Bind("MaxScale", &maxScale_, 1.5f, 0.01f, 0.1f, 5.0f);
    binder_->Bind("MinAnisoScale", &minAnisoScale_, { 0.8f, 0.3f, 0.8f });
    binder_->Bind("MaxAnisoScale", &maxAnisoScale_, { 1.2f, 0.8f, 1.2f });

    binder_->Bind("TerrainWidth", &terrainWidth_, 1024.0f, 1.0f, 10.0f, 5000.0f);
    binder_->Bind("TerrainDepth", &terrainDepth_, 1024.0f, 1.0f, 10.0f, 5000.0f);

    binder_->BindColor("BaseColor", &mat->baseColor, { 1.0f, 1.0f, 1.0f, 1.0f });
    binder_->Bind("Roughness", &mat->roughness, 0.8f, 0.01f, 0.0f, 1.0f);
    binder_->Bind("Metalness", &mat->metalness, 0.0f, 0.01f, 0.0f, 1.0f);
    binder_->Bind("NormalIntensity", &mat->normalIntensity, 1.0f, 0.01f, 0.0f, 5.0f);
    binder_->Bind("EnvMapIntensity", &mat->environmentMapIntensity, 0.5f, 0.01f, 0.0f, 2.0f);

    mat->shadowDensity = 1.0f;
    mat->shadowNormalBias = 0.005f;
    mat->shadowBias = 0.001f;
    mat->shadowSoftness = 1.0f;
    mat->shadowEnvStrength = 1.0f;
    mat->shininess = 32.0f;
    mat->diffuseReflection = 1.0f;

    binder_->Bind("MaxDrawDistance", &cull->maxDrawDistance, 150.0f, 1.0f, 10.0f, 1000.0f);
    binder_->Bind("ThinStartDistance", &cull->thinStartDistance, 100.0f, 1.0f, 10.0f, 1000.0f);
    binder_->Bind("MaxThinningRate", &cull->maxThinningRate, 0.9f, 0.01f, 0.0f, 1.0f);

    // テクスチャリソースのバインド (変更時にコールバック)
    auto onResourceChanged = [this]() { ReloadResources(); };
    binder_->BindTexture("Skybox", &skyboxName_, &skyboxHandle_, "Skybox", TextureType::CubeMap, onResourceChanged);
    binder_->BindTexture("AlbedoMap", &albedoMapName_, &albedoMapHandle_, "white1x1", TextureType::Albedo, onResourceChanged);
    binder_->BindTexture("NormalMap", &normalMapName_, &normalMapHandle_, "white1x1", TextureType::Normal, onResourceChanged);

    binder_->BindTexture("HeightMap", &heightMapName_, &heightMapHandle_, "noise_39", TextureType::Noise);
    binder_->BindTexture("DensityMap", &densityMapName_, &densityMapHandle_, "white1x1", TextureType::Noise);

    // 初回リソース読み込み
    ReloadResources();

    // 初回生成
    GeneratePebbles();
}

void PebbleField::Update()
{
    // 生成パラメータが変更されたかチェック
    if (transform_.translation_.x != prevPosition_.x ||
        transform_.translation_.y != prevPosition_.y ||
        transform_.translation_.z != prevPosition_.z ||
        maxPebblesPerChunk_ != prevMaxPebbles_ ||
        gridSpacing_ != prevGridSpacing_ ||
        minScale_ != prevMinScale_ ||
        maxScale_ != prevMaxScale_ ||
        minAnisoScale_ != prevMinAnisoScale_ || 
        maxAnisoScale_ != prevMaxAnisoScale_ || 
        terrainWidth_ != prevTerrainWidth_ ||
        heightMapName_ != prevHeightMapName_ ||
        densityMapName_ != prevDensityMapName_)
    {
        GeneratePebbles(); // 変更があればGPUで再配置

        // 状態をキャッシュ
        prevPosition_ = transform_.translation_;
        prevMaxPebbles_ = maxPebblesPerChunk_;
        prevGridSpacing_ = gridSpacing_;
        prevMinScale_ = minScale_;
        prevMaxScale_ = maxScale_;
        prevMinAnisoScale_ = minAnisoScale_; 
        prevMaxAnisoScale_ = maxAnisoScale_; 
        prevTerrainWidth_ = terrainWidth_;
        prevHeightMapName_ = heightMapName_;
        prevDensityMapName_ = densityMapName_;
    }

    // 毎フレームのマテリアル・カリングデータを送信
    pebbleSystem_->Update();
}

void PebbleField::Draw() {} 

void PebbleField::DebugDraw()
{
#ifdef ENABLE_IMGUI
    ImGui::Begin("小石プロシージャル");

    if (ImGui::CollapsingHeader("配置設定 (変更で自動再生成)", ImGuiTreeNodeFlags_DefaultOpen))
    {
        binder_->Draw("Position", "中心座標");
        binder_->Draw("TerrainWidth", "地形の幅 (Width)");
        binder_->Draw("TerrainDepth", "地形の奥行き (Depth)");

        binder_->Draw("MaxPebbles", "最大生成数");
        binder_->Draw("GridSpacing", "配置間隔 (小さいほど高密度)");
        binder_->Draw("MinScale", "最小スケール");
        binder_->Draw("MaxScale", "最大スケール");
        binder_->Draw("MinAnisoScale", "非等方スケール(最小)");
        binder_->Draw("MaxAnisoScale", "非等方スケール(最大)");

        ImGui::Separator();
        binder_->Draw("AlbedoMap", "アルベドテクスチャ");
        binder_->Draw("NormalMap", "ノーマルテクスチャ");
        binder_->Draw("Skybox", "反射用スカイボックス");
        binder_->Draw("HeightMap", "ハイトマップ");
        binder_->Draw("DensityMap", "密度マップ (生える場所)");

        if (ImGui::Button("強制再生成 (Generate)"))
        {
            GeneratePebbles();
        }
    }

    if (ImGui::CollapsingHeader("質感・マテリアル"))
    {
        binder_->Draw("BaseColor", "基本色");
        binder_->Draw("Roughness", "粗さ (0=ツルツル, 1=ザラザラ)");
        binder_->Draw("Metalness", "金属度");
        binder_->Draw("NormalIntensity", "法線の強さ(凹凸)");
        binder_->Draw("EnvMapIntensity", "環境反射の強さ");
    }

    if (ImGui::CollapsingHeader("描画距離・カリング"))
    {
        binder_->Draw("MaxDrawDistance", "描画限界距離");
        binder_->Draw("ThinStartDistance", "間引き開始距離");
        binder_->Draw("MaxThinningRate", "最大間引き率");
    }

    ImGui::End();
#endif
}

void PebbleField::ReloadResources()
{
    const FE::ModelData* modelData = FE::ModelManager::GetInstance().Get(pebbleModelName_);

    if (modelData && !modelData->meshes.empty())
    {
        const auto& meshPart = modelData->meshes[0];

        // メッシュを PebbleSystem にセット
        pebbleSystem_->SetResources(
            skyboxName_,
            albedoMapName_,
            normalMapName_,
            meshPart
        );

        // AABB から Bounds を自動計算
        auto* cull = pebbleSystem_->GetCullingData();

        FE::Vector3 minPos = meshPart.localAABB.min;
        FE::Vector3 maxPos = meshPart.localAABB.max;

        // Y中心オフセット
        cull->modelCenterYOffset = (minPos.y + maxPos.y) * 0.5f;

        // 半径
        FE::Vector3 size = { maxPos.x - minPos.x, maxPos.y - minPos.y, maxPos.z - minPos.z };
        cull->modelRadius = std::sqrt(size.x * size.x + size.y * size.y + size.z * size.z) * 0.5f;

        // モデルの AABB が小さすぎたり0だった場合の安全対策
        if (cull->modelRadius < 0.01f)
        {
            cull->modelRadius = 1.0f;
        }
    }
    else
    {
        assert(false && "小石のモデルが見つかりません！");
    }
}

void PebbleField::GeneratePebbles()
{
    PebbleGenerationData genData{};

    // 必要なインスタンス数を計算
    uint32_t gridX = static_cast<uint32_t>(std::ceil(terrainWidth_ / gridSpacing_));
    uint32_t gridZ = static_cast<uint32_t>(std::ceil(terrainDepth_ / gridSpacing_));
    uint32_t neededPebbleCount = gridX * gridZ;

    // バッファの上限を超えないように制限
    genData.maxInstancesPerChunk = std::min(static_cast<uint32_t>(maxPebblesPerChunk_), neededPebbleCount);

    // 生成用のデータを構築
    genData.chunkBasePos = { transform_.translation_.x, transform_.translation_.z };
    genData.terrainCenter = { terrainCenter_.x, terrainCenter_.y };
    genData.terrainWidth = terrainWidth_;
    genData.terrainDepth = terrainDepth_;
    genData.gridSpacing = gridSpacing_; 

    // スケール設定
    genData.minScale = minScale_;
    genData.maxScale = maxScale_;
    genData.minAnisoScale = { minAnisoScale_.x, minAnisoScale_.y, minAnisoScale_.z };
    genData.maxAnisoScale = { maxAnisoScale_.x, maxAnisoScale_.y, maxAnisoScale_.z };


    // System に生成命令を出す
    pebbleSystem_->Generate(genData, heightMapName_, densityMapName_);
}