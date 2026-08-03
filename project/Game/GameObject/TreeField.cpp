#include "pch.h"
#include "TreeField.h"
#include "ImGuiManager.h"
#include "MathUtils.h"
#include "Model.h"

using namespace FE;

TreeField::TreeField(Engine* engine) : GameObject()
{
    engine_ = engine;

    model_ = std::make_unique<Model>(engine_, "tree");
    treeSystem_ = std::make_unique<TreeSystem>(engine_, "noise_39");
    binder_ = std::make_unique<PropertyBinder>(engine_, "TreeField");

    envMapName_ = "skybox";
    envMapHandle_ = TextureManager::GetInstance().Get("skybox");
}

void TreeField::Initialize()
{
    if (model_) {
        // メッシュ0: 幹
        if (model_->GetMaterialCount() > 0) {
            if (auto* m0 = model_->GetMaterialHandle(0)) {
                trunkTextureName_ = m0->textureName;
                trunkNormalName_ = m0->normalMapName;
                envMapName_ = m0->envMapName;
                toonRampName_ = m0->toonRampName;
            }
        }
        // メッシュ1: 葉
        if (model_->GetMaterialCount() > 1) {
            if (auto* m1 = model_->GetMaterialHandle(1)) {
                leafTextureName_ = m1->textureName;
                leafNormalName_ = m1->normalMapName;
                leafRoughnessName_ = m1->metallicRoughnessName;
            }
        }
    }

    binder_->Bind("Position", &transform_.translation_, { 0.0f, 0.0f, 0.0f });
    binder_->Bind("TreeCount", &treeCount_, 500, 1, 10, 5000);
    binder_->Bind("AreaWidth", &areaWidth_, 400.0f, 1.0f, 10.0f, 2000.0f);
    binder_->Bind("AreaDepth", &areaDepth_, 400.0f, 1.0f, 10.0f, 2000.0f);
    binder_->Bind("AreaCenterX", &areaCenter_.x, 0.0f, 1.0f, -2000.0f, 2000.0f);
    binder_->Bind("AreaCenterZ", &areaCenter_.y, 0.0f, 1.0f, -2000.0f, 2000.0f);
    binder_->Bind("MinScale", &minScale_, 0.8f, 0.05f, 0.1f);
    binder_->Bind("MaxScale", &maxScale_, 1.4f, 0.05f, 0.1f);
    binder_->Bind("ColorRandomness", &colorRandomness_, 0.15f, 0.01f, 0.0f, 0.5f);

    binder_->BindTexture("WindMap", &windMapName_, &windMapHandle_, "noise_39", FE::TextureType::Noise, [this]()
        {
            treeSystem_->SetWindMapTexture(windMapName_);
        });

    // --- 葉(Leaf) の設定 ---
    binder_->Bind("GustScale", &gustScale_, 0.05f, 0.01f, 0.0f, 1.0f);
    binder_->Bind("BaseWindStrength", &baseWindStrength_, 0.2f, 0.01f, 0.0f, 5.0f);
    binder_->Bind("GustStrength", &gustStrength_, 1.0f, 0.01f, 0.0f, 5.0f);
    binder_->Bind("TrunkFlex", &trunkFlexibility_, 0.1f, 0.01f, 0.0f, 2.0f);
    binder_->Bind("BranchFlex", &branchFlexibility_, 0.3f, 0.01f, 0.0f, 2.0f);
    binder_->Bind("LeafFlutter", &leafFlutterAmount_, 0.2f, 0.01f, 0.0f, 2.0f);
    binder_->Bind("BackfaceFlatten", &backfaceFlatten_, 0.5f, 0.01f, 0.0f, 1.0f);
    binder_->Bind("DiffuseWrap", &diffuseWrap_, 0.2f, 0.01f, 0.0f, 1.0f);
    binder_->Bind("TransDistortion", &transmissionDistortion_, 0.1f, 0.01f, 0.0f, 1.0f);
    binder_->Bind("TransPower", &transmissionPower_, 5.0f, 0.1f, 1.0f, 20.0f);
    binder_->Bind("SSSStrength", &sssStrength_, 1.0f, 0.01f, 0.0f, 5.0f);
    binder_->BindColor("SSSColor", &sssColor_, { 0.5f, 0.7f, 0.2f });
    binder_->Bind("AlphaCutoff", &alphaCutoff_, 0.5f, 0.01f, 0.0f, 1.0f);
    binder_->Bind("LeafShadowDensity", &leafShadowDensity_, 0.8f, 0.01f, 0.0f, 1.0f);
    binder_->Bind("LeafShadowNormBias", &leafShadowNormalBias_, 0.02f, 0.001f, 0.0f, 0.5f);
    binder_->Bind("LeafShadowBias", &leafShadowBias_, 0.005f, 0.0001f, 0.0f, 0.1f);
    binder_->Bind("TreeHeight", &treeHeight_, 10.0f, 0.1f, 1.0f, 50.0f);
    binder_->Bind("TreeRadius", &treeRadius_, 5.0f, 0.1f, 0.1f, 20.0f);
    binder_->Bind("RoughnessScale", &roughnessScale_, 1.0f, 0.01f, 0.0f, 5.0f);
    binder_->Bind("BaseAO", &baseAO_, 1.0f, 0.01f, 0.0f, 2.0f);
    binder_->Bind("BaseThickness", &baseThickness_, 0.1f, 0.01f, 0.0f, 1.0f);

    // --- 幹(Trunk) の設定 ---
    binder_->BindColor("TrunkColor", &trunkColor_, { 1.0f, 1.0f, 1.0f, 1.0f });
    binder_->BindColor("TrunkSpecColor", &trunkSpecularColor_, { 1.0f, 1.0f, 1.0f, 1.0f });
    binder_->Bind("TrunkRoughness", &trunkRoughness_, 0.8f, 0.01f, 0.0f, 1.0f);
    binder_->Bind("TrunkMetalness", &trunkMetalness_, 0.0f, 0.01f, 0.0f, 1.0f);
    binder_->Bind("TrunkShininess", &trunkShininess_, 10.0f, 0.1f, 0.0f, 100.0f);
    binder_->Bind("TrunkDiffReflect", &trunkDiffuseReflection_, 1.0f, 0.01f, 0.0f, 5.0f);
    binder_->Bind("TrunkEnvMapInt", &trunkEnvironmentMapIntensity_, 1.0f, 0.01f, 0.0f, 5.0f);
    binder_->Bind("TrunkNormalInt", &trunkNormalIntensity_, 1.0f, 0.01f, 0.0f, 5.0f);
    binder_->Bind("TrunkShadowDens", &trunkShadowDensity_, 0.8f, 0.01f, 0.0f, 1.0f);
    binder_->Bind("TrunkShadowBias", &trunkShadowBias_, 0.005f, 0.0001f, 0.0f, 0.1f);
    binder_->Bind("TrunkShadowNBias", &trunkShadowNormalBias_, 0.02f, 0.001f, 0.0f, 0.5f);
    binder_->Bind("TrunkShadowSoft", &trunkShadowSoftness_, 1.0f, 0.01f, 0.0f, 5.0f);
    binder_->Bind("TrunkShadowEnvStr", &trunkShadowEnvStrength_, 0.5f, 0.01f, 0.0f, 1.0f);

    // ==========================================
    // テクスチャの BindTexture 設定
    // ==========================================
    auto OnTextureChanged = [this]() {
        // テクスチャが変更されたら GPU 側の TreeMaterialHandle を更新
        if (treeMaterialHandle_.leafMaterialBuffer) {
            treeMaterialHandle_.leafTextureHandle = leafTextureHandle_;
            treeMaterialHandle_.leafNormalMapHandle = leafNormalHandle_;
            treeMaterialHandle_.leafMetallicRoughnessHandle = leafRoughnessHandle_;
            treeMaterialHandle_.trunkTextureHandle = trunkTextureHandle_;
            treeMaterialHandle_.trunkNormalMapHandle = trunkNormalHandle_;
            treeMaterialHandle_.envMapHandle = envMapHandle_;
            treeMaterialHandle_.toonRampHandle = toonRampHandle_;
        }
        };

    // 1. 風ノイズ (Noise)
    binder_->BindTexture("WindMap", &windMapName_, &windMapHandle_, "noise_39", FE::TextureType::Noise, [this]() {
        treeSystem_->SetWindMapTexture(windMapName_);
        });

    // 2. 葉 (Leaf)
    binder_->BindTexture("LeafTexture", &leafTextureName_, &leafTextureHandle_, leafTextureName_, FE::TextureType::Albedo, OnTextureChanged);
    binder_->BindTexture("LeafNormal", &leafNormalName_, &leafNormalHandle_, leafNormalName_, FE::TextureType::Normal, OnTextureChanged);
    binder_->BindTexture("LeafRoughness", &leafRoughnessName_, &leafRoughnessHandle_, leafRoughnessName_, FE::TextureType::Albedo, OnTextureChanged);

    // 3. 幹 (Trunk)
    binder_->BindTexture("TrunkTexture", &trunkTextureName_, &trunkTextureHandle_, trunkTextureName_, FE::TextureType::Albedo, OnTextureChanged);
    binder_->BindTexture("TrunkNormal", &trunkNormalName_, &trunkNormalHandle_, trunkNormalName_, FE::TextureType::Normal, OnTextureChanged);

    // 4. 共通 (Env / Toon)
    binder_->BindTexture("EnvMap", &envMapName_, &envMapHandle_, envMapName_, FE::TextureType::CubeMap, OnTextureChanged);
    binder_->BindTexture("ToonRamp", &toonRampName_, &toonRampHandle_, toonRampName_, FE::TextureType::Toon, OnTextureChanged);

    // 初期化値の記憶
    prevPosition_ = transform_.translation_;
    prevTreeCount_ = treeCount_;
    prevAreaWidth_ = areaWidth_;
    prevAreaDepth_ = areaDepth_;
    prevAreaCenter_ = areaCenter_;
    prevMinScale_ = minScale_;
    prevMaxScale_ = maxScale_;
    prevColorRandomness_ = colorRandomness_;

    treeSystem_->SetWindMapTexture(windMapName_);

    // 初回配置生成
    GenerateTrees();
}

void TreeField::Update()
{
    // 配置パラメータが変わった時だけ再生成
    if (transform_.translation_.x != prevPosition_.x ||
        transform_.translation_.y != prevPosition_.y ||
        transform_.translation_.z != prevPosition_.z ||
        treeCount_ != prevTreeCount_ ||
        areaWidth_ != prevAreaWidth_ ||
        areaDepth_ != prevAreaDepth_ ||
        areaCenter_.x != prevAreaCenter_.x ||
        areaCenter_.y != prevAreaCenter_.y ||
        minScale_ != prevMinScale_ ||
        maxScale_ != prevMaxScale_ ||
        colorRandomness_ != prevColorRandomness_)
    {
        GenerateTrees();

        prevPosition_ = transform_.translation_;
        prevTreeCount_ = treeCount_;
        prevAreaWidth_ = areaWidth_;
        prevAreaDepth_ = areaDepth_;
        prevAreaCenter_ = areaCenter_;
        prevMinScale_ = minScale_;
        prevMaxScale_ = maxScale_;
        prevColorRandomness_ = colorRandomness_;
    }

    // 毎フレーム UIなどの変更を定数バッファに流し込む
    UpdateMaterials();

    // TreeSystem の Update（Submit処理）を実行
    treeSystem_->Update();
}

void TreeField::UpdateMaterials()
{
    if (!treeMaterialHandle_.leafMaterialBuffer) return;

    // 葉のパラメータ更新
    LeafMaterialData leafData{};
    leafData.gustScale = gustScale_;
    leafData.baseWindStrength = baseWindStrength_;
    leafData.gustStrength = gustStrength_;
    leafData.trunkFlexibility = trunkFlexibility_;
    leafData.branchFlexibility = branchFlexibility_;
    leafData.leafFlutterAmount = leafFlutterAmount_;
    leafData.backfaceFlatten = backfaceFlatten_;
    leafData.diffuseWrap = diffuseWrap_;
    leafData.transmissionDistortion = transmissionDistortion_;
    leafData.transmissionPower = transmissionPower_;
    leafData.sssStrength = sssStrength_;
    leafData.alphaCutoff = alphaCutoff_;
    leafData.shadowDensity = leafShadowDensity_;
    leafData.sssColor = sssColor_;
    leafData.shadowNormalBias = leafShadowNormalBias_;
    leafData.treeHeight = treeHeight_;
    leafData.treeRadius = treeRadius_;
    leafData.isLeaf = 1.0f; // 固定
    leafData.shadowBias = leafShadowBias_;
    leafData.roughnessScale = roughnessScale_;
    leafData.baseAO = baseAO_;
    leafData.baseThickness = baseThickness_;

    treeSystem_->UpdateLeafMaterial(treeMaterialHandle_, leafData);

    // 幹のパラメータ更新
    TrunkMaterialData trunkData{};
    trunkData.color = trunkColor_;
    trunkData.specularColor = trunkSpecularColor_;
    trunkData.enableLighting = trunkEnableLighting_;
    trunkData.lightMode = trunkLightMode_;
    trunkData.enableNormalMap = trunkEnableNormalMap_;
    trunkData.addShadow = trunkAddShadow_;
    trunkData.roughness = trunkRoughness_;
    trunkData.metalness = trunkMetalness_;
    trunkData.shininess = trunkShininess_;
    trunkData.diffuseReflection = trunkDiffuseReflection_;
    trunkData.shadowDensity = trunkShadowDensity_;
    trunkData.shadowBias = trunkShadowBias_;
    trunkData.shadowNormalBias = trunkShadowNormalBias_;
    trunkData.shadowSoftness = trunkShadowSoftness_;
    trunkData.environmentMapIntensity = trunkEnvironmentMapIntensity_;
    trunkData.shadowEnvStrength = trunkShadowEnvStrength_;
    trunkData.normalIntensity = trunkNormalIntensity_;
    trunkData.pad = 0.0f;

    treeSystem_->UpdateTrunkMaterial(treeMaterialHandle_, trunkData);
}

void TreeField::GenerateTrees()
{
    treeSystem_->Clear();

    if (!model_) return;

    const ModelData* modelData = model_->GetModelData();
    if (!modelData) return;

    // ========================================================
    // ★ 共有マテリアルハンドルの生成（初回または更新時）
    // ========================================================
    if (!treeMaterialHandle_.leafMaterialBuffer)
    {
        // BindTexture で既に取得・同期されているハンドルを渡す
        treeMaterialHandle_ = treeSystem_->CreateTreeMaterial(
            LeafMaterialData{}, TrunkMaterialData{},
            leafTextureHandle_, trunkTextureHandle_,
            leafNormalHandle_, trunkNormalHandle_
        );

        // その他の拡張ハンドルを設定
        treeMaterialHandle_.leafMetallicRoughnessHandle = leafRoughnessHandle_;
        treeMaterialHandle_.envMapHandle = envMapHandle_;
        treeMaterialHandle_.toonRampHandle = toonRampHandle_;
    }

    // ========================================================
    // インスタンス配置ループ
    // ========================================================
    for (int i = 0; i < treeCount_; ++i)
    {
        float rx = Math::RandomFloat(-areaWidth_ * 0.5f, areaWidth_ * 0.5f) + areaCenter_.x + transform_.translation_.x;
        float rz = Math::RandomFloat(-areaDepth_ * 0.5f, areaDepth_ * 0.5f) + areaCenter_.y + transform_.translation_.z;
        float ry = transform_.translation_.y;
        if (terrain_) {
            ry = terrain_->GetHeight(rx, rz);
        }

        float scale = Math::RandomFloat(minScale_, maxScale_);
        float rotY = Math::RandomFloat(0.0f, Math::PI * 2.0f);

        WorldTransform treeTransform{};
        treeTransform.translation_ = { rx, ry, rz };
        treeTransform.rotation_ = { 0.0f, rotY, 0.0f };
        treeTransform.scale_ = { scale, scale, scale };
        treeTransform.UpdateMatrix();

        // 色ムラ
        Vector4 colorVar = { 1.0f, 1.0f, 1.0f, 1.0f };
        colorVar.x += Math::RandomFloat(-colorRandomness_, colorRandomness_);
        colorVar.y += Math::RandomFloat(-colorRandomness_, colorRandomness_);
        colorVar.z += Math::RandomFloat(-colorRandomness_, colorRandomness_);

        // TreeSystem に追加（古い std::vector<MaterialHandle> から treeMaterialHandle_ に変更！）
        treeSystem_->AddInstance(treeTransform, *modelData, treeMaterialHandle_, colorVar, 1.0f);
    }
}

void TreeField::DebugDraw()
{
#ifdef IS_DEVELOPMENT
    ImGui::Begin("森林");

    if (ImGui::CollapsingHeader("配置設定 (変更で自動再生成)", ImGuiTreeNodeFlags_DefaultOpen))
    {
        binder_->Draw("Position", "中心座標");
        binder_->Draw("TreeCount", "木の配置本数");
        binder_->Draw("AreaWidth", "配置範囲 (幅 X)");
        binder_->Draw("AreaDepth", "配置範囲 (奥行 Z)");
        binder_->Draw("AreaCenterX", "配置中心 X");
        binder_->Draw("AreaCenterZ", "配置中心 Z");

        ImGui::Separator();
        binder_->Draw("MinScale", "最小スケール");
        binder_->Draw("MaxScale", "最大スケール");
        binder_->Draw("ColorRandomness", "色のバリエーション幅");

        ImGui::Separator();
        ImGui::Text("[ 環境ノイズ ]");
        binder_->Draw("WindMap", "風のノイズテクスチャ");

        if (ImGui::Button("強制再生成 (Re-Generate)"))
        {
            GenerateTrees();
        }
    }

    if (ImGui::CollapsingHeader("葉 (Leaf) パラメータ"))
    {
        ImGui::Text("[ テクスチャ ]");
        binder_->Draw("LeafTexture", "アルベド (Albedo)");
        binder_->Draw("LeafNormal", "ノーマルマップ (Normal)");
        binder_->Draw("LeafRoughness", "メタリック/ラフネス (Roughness)");

        ImGui::Separator();
        ImGui::Text("[ 風・揺れ (VS) ]");
        binder_->Draw("TreeHeight", "木の高さ (基準)");
        binder_->Draw("TreeRadius", "木の半径 (基準)");
        binder_->Draw("GustScale", "突風ノイズスケール");
        binder_->Draw("BaseWindStrength", "基本の風の強さ");
        binder_->Draw("GustStrength", "突風の強さ");
        binder_->Draw("TrunkFlex", "幹のしなりやすさ");
        binder_->Draw("BranchFlex", "枝のしなりやすさ");
        binder_->Draw("LeafFlutter", "葉のバタつき");

        ImGui::Separator();
        ImGui::Text("[ 質感・透過 (PS) ]");
        binder_->Draw("BackfaceFlatten", "裏面法線の平坦化");
        binder_->Draw("DiffuseWrap", "ディフューズラップ");
        binder_->Draw("TransDistortion", "透過光の歪み");
        binder_->Draw("TransPower", "透過光のシャープさ");
        binder_->Draw("SSSStrength", "SSS強度");
        binder_->Draw("SSSColor", "SSSカラー");
        binder_->Draw("AlphaCutoff", "アルファカットオフ");
        binder_->Draw("RoughnessScale", "ラフネス補正");
        binder_->Draw("BaseAO", "ベースAO");
        binder_->Draw("BaseThickness", "葉の厚み");

        ImGui::Separator();
        ImGui::Text("[ 影 (Shadow) ]");
        binder_->Draw("LeafShadowDensity", "影の濃さ");
        binder_->Draw("LeafShadowNormBias", "シャドウノーマルバイアス");
        binder_->Draw("LeafShadowBias", "シャドウ深度バイアス");
    }

    if (ImGui::CollapsingHeader("幹 (Trunk) パラメータ"))
    {
        ImGui::Text("[ テクスチャ ]");
        binder_->Draw("TrunkTexture", "アルベド (Albedo)");
        binder_->Draw("TrunkNormal", "ノーマルマップ (Normal)");

        ImGui::Separator();
        ImGui::Text("[ 質感 (PS) ]");
        binder_->Draw("TrunkColor", "カラー");
        binder_->Draw("TrunkSpecColor", "スペキュラカラー");
        binder_->Draw("TrunkRoughness", "ラフネス");
        binder_->Draw("TrunkMetalness", "メタルネス");
        binder_->Draw("TrunkShininess", "ハイライト鋭さ(Shininess)");
        binder_->Draw("TrunkDiffReflect", "ディフューズ反射");
        binder_->Draw("TrunkNormalInt", "ノーマルマップ強度");
        binder_->Draw("TrunkEnvMapInt", "環境マップ反射強度");

        ImGui::Separator();
        ImGui::Text("[ 影 (Shadow) ]");
        binder_->Draw("TrunkShadowDens", "影の濃さ");
        binder_->Draw("TrunkShadowBias", "シャドウ深度バイアス");
        binder_->Draw("TrunkShadowNBias", "シャドウノーマルバイアス");
        binder_->Draw("TrunkShadowSoft", "影のソフトネス");
        binder_->Draw("TrunkShadowEnvStr", "環境マップの影への影響");
    }

    // 環境・シェーディング共通のヘッダーを新設
    if (ImGui::CollapsingHeader("環境・シェーディング共通"))
    {
        ImGui::Text("[ グローバルテクスチャ ]");
        binder_->Draw("EnvMap", "環境マップ (CubeMap)");
        binder_->Draw("ToonRamp", "トゥーンランプ (Toon)");
    }

    ImGui::Separator();
    ImGui::Text("現在の描画本数: %u", treeSystem_->GetInstanceCount());

    ImGui::End();
#endif
}