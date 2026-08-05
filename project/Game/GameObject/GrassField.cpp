#include "pch.h"
#include "GrassField.h"
#include "ImGuiManager.h"
#include "Player.h"

using namespace FE;

GrassField::GrassField(FE::Engine* engine, Player* player) : FE::GameObject()
{
    engine_ = engine;

    grassSystem_ = std::make_unique<FE::GrassSystem>(engine_, "noise_39");
    binder_ = std::make_unique<FE::PropertyBinder>(engine_, "GrassField");
    player_ = player;
}

void GrassField::Initialize()
{
    auto* grassMat = grassSystem_->GetMaterialData();
    auto* cullingData = grassSystem_->GetCullingData();

    binder_->Bind("Position", &transform_.translation_, { 0.0f, 0.0f, 0.0f });
    binder_->Bind("BaseScale", &baseScale_, 1.0f, 0.01f, 0.1f, 5.0f);
    binder_->Bind("MaxGrassCount", &maxGrassPerChunk_, 300000, 1000, 1000, 2000000);
    binder_->Bind("GridSpacing", &gridSpacing_, 0.5f, 0.01f, 0.05f, 2.0f);

    binder_->Bind("MinHeight", &minHeight_, 0.8f, 0.01f, 0.1f, 5.0f);
    binder_->Bind("MaxHeight", &maxHeight_, 1.5f, 0.01f, 0.1f, 5.0f);
    binder_->Bind("MinWidth", &minWidth_, 0.8f, 0.01f, 0.1f, 5.0f);
    binder_->Bind("MaxWidth", &maxWidth_, 1.5f, 0.01f, 0.1f, 5.0f);

    binder_->Bind("TerrainCenterX", &terrainCenter_.x, 0.0f, 1.0f, -2000.0f, 2000.0f);
    binder_->Bind("TerrainCenterZ", &terrainCenter_.y, 0.0f, 1.0f, -2000.0f, 2000.0f);
    binder_->Bind("TerrainWidth", &terrainWidth_, 500.0f, 1.0f, 10.0f, 5000.0f);
    binder_->Bind("TerrainDepth", &terrainDepth_, 500.0f, 1.0f, 10.0f, 5000.0f);

    binder_->BindColor("RootColor", &grassMat->rootColor, { 0.1f, 0.2f, 0.05f });
    binder_->Bind("GrassRootAO", &grassMat->grassRootAO, 0.3f, 0.05f, 0.0f, 1.0f);

    binder_->BindColor("TipColor", &grassMat->tipColor, { 0.4f, 0.7f, 0.2f });
    binder_->Bind("GrassNormalBlend", &grassMat->grassNormalBlend, 0.7f, 0.05f, 0.0f, 1.0f);

    binder_->BindColor("SSSColor", &grassMat->sssColor, { 0.7f, 0.9f, 0.3f });
    binder_->Bind("SSSStrength", &grassMat->sssStrength, 1.5f, 0.1f, 0.0f, 5.0f);

    binder_->Bind("ColorVariation", &grassMat->colorVariation, 0.5f, 0.05f, 0.0f, 1.0f);
    binder_->Bind("SpecularStrength", &grassMat->specularStrength, 0.2f, 0.01f, 0.0f, 1.0f);
    binder_->Bind("SpecularShininess", &grassMat->specularShininess, 40.0f, 1.0f, 10.0f, 200.0f);
    binder_->Bind("Wetness", &grassMat->wetness, 0.0f, 0.05f, 0.0f, 1.0f);

    binder_->Bind("WindDirX", &grassMat->windDir.x, 1.0f, 0.05f, -1.0f, 1.0f);
    binder_->Bind("WindDirY", &grassMat->windDir.y, 0.8f, 0.05f, -1.0f, 1.0f);
    binder_->Bind("WindSpeed", &grassMat->windSpeed, 1.5f, 0.1f, 0.0f, 10.0f);
    binder_->Bind("BaseWindStrength", &grassMat->baseWindStrength, 0.3f, 0.05f, 0.0f, 2.0f);
    binder_->Bind("GustScale", &grassMat->gustScale, 0.03f, 0.005f, 0.001f, 0.5f);
    binder_->Bind("GustStrength", &grassMat->gustStrength, 1.2f, 0.1f, 0.0f, 5.0f);
    binder_->Bind("WindFlattenStrength", &grassMat->windFlattenStrength, 1.0f, 0.05f, 0.0f, 3.0f);
    binder_->Bind("FlutterAmount", &grassMat->flutterAmount, 0.15f, 0.01f, 0.0f, 1.0f);
    binder_->Bind("WindHighlightStrength", &grassMat->windHighlightStrength, 0.4f, 0.05f, 0.0f, 1.0f);

    binder_->Bind("InteractRadius", &grassMat->interactRadius, 1.2f, 0.1f, 0.1f, 5.0f);
    binder_->Bind("InteractStrength", &grassMat->interactStrength, 1.0f, 0.1f, 0.0f, 3.0f);
    binder_->Bind("ShadowDensity", &grassMat->shadowDensity, 0.8f, 0.05f, 0.0f, 1.0f);
    binder_->Bind("ShadowBias", &grassMat->shadowBias, 0.005f, 0.001f, 0.0f, 0.05f);
    binder_->Bind("ShadowNormalBias", &grassMat->shadowNormalBias, 0.02f, 0.001f, 0.0f, 0.1f);

    binder_->Bind("MaxDrawDistance", &cullingData->maxDrawDistance, 150.0f, 1.0f, 10.0f, 1000.0f);
    binder_->Bind("ThinStartDistance", &cullingData->thinStartDistance, 50.0f, 1.0f, 10.0f, 500.0f);
    binder_->Bind("MaxThinningRate", &cullingData->maxThinningRate, 0.8f, 0.05f, 0.0f, 0.99f);
    binder_->Bind("MaxWidthMultiplier", &cullingData->maxWidthMultiplier, 2.5f, 0.1f, 1.0f, 5.0f);
    binder_->Bind("LodDistance1", &cullingData->lodDistance1, 20.0f, 1.0f, 5.0f, 100.0f);
    binder_->Bind("LodDistance2", &cullingData->lodDistance2, 50.0f, 1.0f, 10.0f, 200.0f);

    binder_->BindTexture("HeightMap", &heightMapName_, &heightMapHandle_, "noise_39", TextureType::Noise);
    binder_->BindTexture("DensityMap", &densityMapName_, &densityMapHandle_, "white1x1", TextureType::Noise);

    binder_->BindTexture("WindMap", &windMapName_, &windMapHandle_, "noise_39", FE::TextureType::Noise, [this]() 
        {
        // 風のテクスチャが変更されたら、GrassSystemに新しいテクスチャを通知
        grassSystem_->SetWindMapTexture(windMapName_);
        });

    // 記憶初期化
    prevPosition_ = transform_.translation_;
    prevMaxGrassPerChunk_ = maxGrassPerChunk_;
    prevGridSpacing_ = gridSpacing_;
    prevBaseScale_ = baseScale_;
    prevMinHeight_ = minHeight_;
    prevMaxHeight_ = maxHeight_;
    prevMinWidth_ = minWidth_;
    prevMaxWidth_ = maxWidth_;
    prevHeightMapName_ = heightMapName_;
    prevDensityMapName_ = densityMapName_;
    prevTerrainCenter_ = terrainCenter_;
    prevTerrainWidth_ = terrainWidth_;
    prevTerrainDepth_ = terrainDepth_;

    // 草システムの風テクスチャ初期化
    grassSystem_->SetWindMapTexture(windMapName_);

    // 初回生成
    GenerateGrass();
}

void GrassField::Update()
{
    // 形状や配置に関するパラメータが変わった時だけ再生成
    if (transform_.translation_.x != prevPosition_.x ||
        transform_.translation_.y != prevPosition_.y ||
        transform_.translation_.z != prevPosition_.z ||
        maxGrassPerChunk_ != prevMaxGrassPerChunk_ ||
        gridSpacing_ != prevGridSpacing_ ||
        baseScale_ != prevBaseScale_ ||
        minHeight_ != prevMinHeight_ ||
        maxHeight_ != prevMaxHeight_ ||
        minWidth_ != prevMinWidth_ ||
        maxWidth_ != prevMaxWidth_ ||
        heightMapName_ != prevHeightMapName_ ||
        densityMapName_ != prevDensityMapName_ ||
        terrainCenter_.x != prevTerrainCenter_.x ||
        terrainCenter_.y != prevTerrainCenter_.y ||
        terrainWidth_ != prevTerrainWidth_ ||
        terrainDepth_ != prevTerrainDepth_)
    {
        GenerateGrass();

        prevPosition_ = transform_.translation_;
        prevMaxGrassPerChunk_ = maxGrassPerChunk_;
        prevGridSpacing_ = gridSpacing_;
        prevBaseScale_ = baseScale_;
        prevMinHeight_ = minHeight_;
        prevMaxHeight_ = maxHeight_;
        prevMinWidth_ = minWidth_;
        prevMaxWidth_ = maxWidth_;
        prevHeightMapName_ = heightMapName_;
        prevDensityMapName_ = densityMapName_;
        prevTerrainCenter_ = terrainCenter_;
        prevTerrainWidth_ = terrainWidth_;
        prevTerrainDepth_ = terrainDepth_;
    }

    // プレイヤー座標をマテリアルに伝える
    if (player_)
    {
        auto pos = player_->animationModel_->GetTransform().translation_;
        grassSystem_->GetMaterialData()->playerPos = { pos.x, pos.y, pos.z };
    }

    grassSystem_->Update();
}

void GrassField::Draw()
{

}

void GrassField::DebugDraw()
{
#ifdef IS_DEVELOPMENT
    ImGui::Begin("草むら");

    if (ImGui::CollapsingHeader("配置設定 (変更で自動再生成)", ImGuiTreeNodeFlags_DefaultOpen))
    {
        binder_->Draw("Position", "中心座標");
        binder_->Draw("BaseScale", "全体の大きさ");
        binder_->Draw("MaxGrassCount", "最大草数");
        binder_->Draw("GridSpacing", "草の間隔 (小さいほど高密度)");

        ImGui::Separator();
        ImGui::Text("地形フィッティング");
        binder_->Draw("TerrainWidth", "地形の幅 (Xサイズ)");
        binder_->Draw("TerrainDepth", "地形の奥行き (Zサイズ)");
        binder_->Draw("TerrainCenterX", "地形の中心 X");
        binder_->Draw("TerrainCenterZ", "地形の中心 Z");

        ImGui::Separator();
        binder_->Draw("MinHeight", "最小の高さ");
        binder_->Draw("MaxHeight", "最大の高さ");
        binder_->Draw("MinWidth", "最小の太さ");
        binder_->Draw("MaxWidth", "最大の太さ");

        ImGui::Separator();
        ImGui::Text("マップテクスチャ");
        binder_->Draw("HeightMap", "ハイトマップ(高さ)");
        binder_->Draw("DensityMap", "密度マップ(生える場所)");

        if (ImGui::Button("強制再生成 (Generate)"))
        {
            GenerateGrass();
        }
    }

    if (ImGui::CollapsingHeader("質感・ライティング"))
    {
        binder_->Draw("RootColor", "根本の色");
        binder_->Draw("GrassRootAO", "根本のAO(暗さ)");

        ImGui::Separator();
        binder_->Draw("TipColor", "先端の色");
        binder_->Draw("ColorVariation", "草原全体の色ムラ");

        ImGui::Separator();
        binder_->Draw("GrassNormalBlend", "法線の上向きブレンド (最重要)");
        binder_->Draw("SSSColor", "透過光(SSS)の色");
        binder_->Draw("SSSStrength", "透過光(SSS)の強さ");

        ImGui::Separator();
        binder_->Draw("SpecularStrength", "ハイライトの基本強度");
        binder_->Draw("SpecularShininess", "ハイライトの鋭さ");
        binder_->Draw("Wetness", "濡れ具合");
    }

    if (ImGui::CollapsingHeader("風の挙動"))
    {
        ImGui::Text("風テクスチャ");
        binder_->Draw("WindMap", "風のノイズテクスチャ");
        ImGui::Separator();
        binder_->Draw("WindDirX", "風向き X");
        binder_->Draw("WindDirY", "風向き Z(Y)");

        binder_->Draw("WindSpeed", "風の移動速度");
        binder_->Draw("BaseWindStrength", "常時吹くそよ風の強さ");

        ImGui::Separator();
        binder_->Draw("GustScale", "突風ノイズのスケール");
        binder_->Draw("GustStrength", "突風の強さ(水平方向)");
        binder_->Draw("WindFlattenStrength", "突風時の押し潰し(下方向)");
        binder_->Draw("FlutterAmount", "葉先の震えの強さ");
        binder_->Draw("WindHighlightStrength", "風による光沢変化(シルバーライニング)");
    }

    if (ImGui::CollapsingHeader("インタラクション"))
    {
        binder_->Draw("InteractRadius", "かき分ける半径");
        binder_->Draw("InteractStrength", "押し倒す強さ");
    }

    if (ImGui::CollapsingHeader("影の設定"))
    {
        binder_->Draw("ShadowDensity", "影の濃さ");
        binder_->Draw("ShadowBias", "深度バイアス");
        binder_->Draw("ShadowNormalBias", "法線バイアス");
    }

    if (ImGui::CollapsingHeader("カリング・LOD設定"))
    {
        binder_->Draw("MaxDrawDistance", "最大描画距離 (これより遠い草は消去)");

        ImGui::Separator();
        binder_->Draw("ThinStartDistance", "間引き開始距離");
        binder_->Draw("MaxThinningRate", "最大間引き率 (1.0に近いほど消える)");
        binder_->Draw("MaxWidthMultiplier", "間引き時の太さ補正倍率");

        ImGui::Separator();
        binder_->Draw("LodDistance1", "LOD1の距離 (高ポリ境界)");
        binder_->Draw("LodDistance2", "LOD2の距離 (中ポリ境界)");
    }

    ImGui::End();
#endif
}

void GrassField::GenerateGrass()
{
    // GPUで草を一括生成するための設定データを作成
    GrassGenerationData genData{};

    // 指定された範囲を敷き詰めるために必要な草の総数を自動計算
    uint32_t gridX = static_cast<uint32_t>(std::ceil(terrainWidth_ / gridSpacing_));
    uint32_t gridZ = static_cast<uint32_t>(std::ceil(terrainDepth_ / gridSpacing_));
    uint32_t neededGrassCount = gridX * gridZ;

    // バッファの上限 (maxGrassPerChunk_) を超えないように制限
    genData.maxGrassPerChunk = FE::Math::MyMin((int)neededGrassCount, maxGrassPerChunk_);

    // 配置のオフセット
    genData.chunkBasePos = { transform_.translation_.x, transform_.translation_.z };

    genData.terrainWidth = terrainWidth_;
    genData.terrainDepth = terrainDepth_;
    genData.maxGrassPerChunk = maxGrassPerChunk_;
    genData.gridSpacing = gridSpacing_;

    // スケールを掛け合わせて最終的な高さを決定
    genData.minHeight = minHeight_ * baseScale_;
    genData.maxHeight = maxHeight_ * baseScale_;
    genData.minWidth = minWidth_ * baseScale_;
    genData.maxWidth = maxWidth_ * baseScale_;

    genData.terrainCenter = terrainCenter_;
    genData.terrainWidth = terrainWidth_;
    genData.terrainDepth = terrainDepth_;

    grassSystem_->Generate(genData, heightMapName_, densityMapName_);
}