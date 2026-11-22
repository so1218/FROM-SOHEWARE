#include "pch.h"
#include "GrassField.h"
#include "ImGuiManager.h"
#include "TextureManager.h"
#include "StringUtils.h"
#include "RendererManager.h"

using namespace FS;

GrassField::GrassField(FS::Engine* engine, FS::Terrain* terrain, const std::string& groupName) : FS::GameObject(), terrain_(terrain)
{
    engine_ = engine;

    grassSystem_ = std::make_unique<FS::GrassSystem>(engine_, "noise_39");
    binder_ = std::make_unique<FS::PropertyBinder>(engine_, groupName);
}

void GrassField::Initialize()
{
    auto* grassMat = grassSystem_->GetMaterialData();
    auto* cullingData = grassSystem_->GetCullingData();

    binder_->Bind("BaseScale", &baseScale_, 1.0f, 0.01f, 0.1f, 5.0f);
    binder_->Bind("MaxGrassCount", &maxGrassCount_, 300000, 1000, 1000, 2000000);
    binder_->Bind("GridSpacing", &gridSpacing_, 0.5f, 0.01f, 0.05f, 2.0f);

    binder_->Bind("MinHeight", &minHeight_, 0.8f, 0.01f, 0.1f, 5.0f);
    binder_->Bind("MaxHeight", &maxHeight_, 1.5f, 0.01f, 0.1f, 5.0f);
    binder_->Bind("MinWidth", &minWidth_, 0.8f, 0.01f, 0.1f, 5.0f);
    binder_->Bind("MaxWidth", &maxWidth_, 1.5f, 0.01f, 0.1f, 5.0f);

    binder_->BindColor("RootColor", &grassMat->rootColor, { 0.1f, 0.2f, 0.05f });

    binder_->BindColor("TipColor", &grassMat->tipColor, { 0.4f, 0.7f, 0.2f });
    binder_->Bind("GrassNormalBlend", &grassMat->grassNormalBlend, 0.7f, 0.05f, 0.0f, 1.0f);

    binder_->BindColor("SSSColor", &grassMat->sssColor, { 0.7f, 0.9f, 0.3f });
    binder_->Bind("SSSStrength", &grassMat->sssStrength, 1.5f, 0.1f, 0.0f, 5.0f);


    binder_->Bind("WindSpeedMult", &grassMat->windSpeedMultiplier, 1.0f, 0.05f, 0.0f, 5.0f);
    binder_->Bind("GustScale", &grassMat->gustScale, 0.03f, 0.005f, 0.001f, 0.5f);
    binder_->Bind("GustStrength", &grassMat->gustStrength, 1.2f, 0.1f, 0.0f, 5.0f);
    binder_->Bind("WindFlattenStrength", &grassMat->windFlattenStrength, 1.0f, 0.05f, 0.0f, 3.0f);
    binder_->Bind("FlutterAmount", &grassMat->flutterAmount, 0.15f, 0.01f, 0.0f, 1.0f);

    binder_->Bind("InteractStrength", &grassMat->interactStrength, 1.0f, 0.1f, 0.0f, 5.0f);
    binder_->Bind("FlattenFactor", &grassMat->flattenFactor, 2.0f, 0.1f, 0.0f, 5.0f);
    binder_->Bind("TrailFlattenWeight", &grassMat->trailFlattenWeight, 0.8f, 0.05f, 0.0f, 1.0f);
    binder_->Bind("ShadowDensity", &grassMat->shadowDensity, 0.8f, 0.05f, 0.0f, 1.0f);
    binder_->Bind("ShadowBias", &grassMat->shadowBias, 0.005f, 0.001f, 0.0f, 0.05f);
    binder_->Bind("ShadowNormalBias", &grassMat->shadowNormalBias, 0.02f, 0.001f, 0.0f, 0.1f);

    binder_->Bind("NearFadeMinDist", &grassMat->nearFadeMinDist, 0.3f, 0.05f, 0.0f, 3.0f);
    binder_->Bind("NearFadeMaxDist", &grassMat->nearFadeMaxDist, 1.2f, 0.05f, 0.1f, 5.0f);
    binder_->Bind("MaxDrawDistance", &cullingData->maxDrawDistance, 150.0f, 1.0f, 10.0f, 1000.0f);
    binder_->Bind("ThinStartDistance", &cullingData->thinStartDistance, 50.0f, 1.0f, 10.0f, 500.0f);
    binder_->Bind("MaxThinningRate", &cullingData->maxThinningRate, 0.8f, 0.05f, 0.0f, 0.99f);
    binder_->Bind("MaxWidthMultiplier", &cullingData->maxWidthMultiplier, 2.5f, 0.1f, 1.0f, 5.0f);
    binder_->Bind("LodDistance1", &cullingData->lodDistance1, 20.0f, 1.0f, 5.0f, 100.0f);
    binder_->Bind("LodDistance2", &cullingData->lodDistance2, 50.0f, 1.0f, 10.0f, 200.0f);

    binder_->Bind("DensityThreshold", &densityThreshold_, 0.5f, 0.01f, 0.0f, 1.0f);

    // 延焼
    GrassFireConstants& fire = engine_->GetRendererManager()->GetGrassFireSettings();
    binder_->Bind("Fire_BurnDuration", &fire.burnDuration, 6.0f, 0.1f, 0.5f, 60.0f);
    binder_->Bind("Fire_SpreadRate", &fire.spreadRate, 2.5f, 0.05f, 0.0f, 30.0f);
    binder_->Bind("Fire_MaxSpreadDistance", &fire.maxSpreadDistance, 25.0f, 0.5f, 0.0f, 1000.0f);
    binder_->Bind("Fire_WindSpreadBias", &fire.windSpreadBias, 1.5f, 0.05f, 0.0f, 10.0f);
    binder_->Bind("Fire_RainSuppression", &fire.rainSuppression, 0.9f, 0.01f, 0.0f, 1.0f);
    binder_->Bind("Fire_RainSpreadDistanceRate", &fire.rainSpreadDistanceRate, 0.35f, 0.01f, 0.0f, 1.0f);
    binder_->Bind("Fire_CoolingRate", &fire.coolingRate, 0.5f, 0.01f, 0.0f, 10.0f);
    binder_->Bind("Fire_CharHeightRatio", &fire.charHeightRatio, 0.15f, 0.01f, 0.0f, 1.0f);
    binder_->BindColor("Fire_ScorchColor", &fire.scorchColor, { 0.35f, 0.25f, 0.1f });
    binder_->BindColor("Fire_CharColor", &fire.charColor, { 0.03f, 0.025f, 0.02f });
    binder_->Bind("Fire_GroundScorchDarkness", &fire.groundScorchDarkness, 0.85f, 0.01f, 0.0f, 1.0f);
    binder_->BindTexture("DensityMap", &densityMapName_, &densityMapHandle_, "white1x1", TextureType::Noise);

    binder_->BindTexture("WindMap", &windMapName_, &windMapHandle_, "noise_39", FS::TextureType::Noise, [this]() 
        {
        grassSystem_->SetWindMapTexture(windMapName_);
        });

    grassSystem_->SetWindMapTexture(windMapName_);
    GenerateGrass();
}

void GrassField::Update()
{
    // 設定か地形が変わったときだけ作り直す
    if (needsRegenerate_ || terrain_->GetShapeVersion() != generatedTerrainVersion_)
    {
        GenerateGrass();
    }

    grassSystem_->Update();
}

void GrassField::Draw()
{

}

void GrassField::DebugGui()
{
    ImGui::Begin("草むら");

    // 生える範囲とハイトマップは地形から取る
    if (ImGui::CollapsingHeader("配置設定 (変更で自動再生成)", ImGuiTreeNodeFlags_DefaultOpen))
    {
        needsRegenerate_ |= binder_->Draw("BaseScale", "全体の大きさ");
        needsRegenerate_ |= binder_->Draw("MaxGrassCount", "最大草数");
        needsRegenerate_ |= binder_->Draw("GridSpacing", "草の間隔");

        ImGui::Separator();
        needsRegenerate_ |= binder_->Draw("MinHeight", "最小の高さ");
        needsRegenerate_ |= binder_->Draw("MaxHeight", "最大の高さ");
        needsRegenerate_ |= binder_->Draw("MinWidth", "最小の太さ");
        needsRegenerate_ |= binder_->Draw("MaxWidth", "最大の太さ");

        ImGui::Separator();
        needsRegenerate_ |= binder_->Draw("DensityMap", "密度マップ");
        needsRegenerate_ |= binder_->Draw("DensityThreshold", "生やす密度のしきい値 (この値以上に生やす)");

        if (ImGui::Button("強制再生成"))
        {
            needsRegenerate_ = true;
        }
    }

    if (ImGui::CollapsingHeader("質感・ライティング"))
    {
        binder_->Draw("RootColor", "根本の色");

        ImGui::Separator();
        binder_->Draw("TipColor", "先端の色");

        ImGui::Separator();
        binder_->Draw("GrassNormalBlend", "法線の上向きブレンド");
        binder_->Draw("SSSColor", "透過光(SSS)の色");
        binder_->Draw("SSSStrength", "透過光(SSS)の強さ");
    }

    if (ImGui::CollapsingHeader("風の挙動"))
    {
        ImGui::Text("風テクスチャ");
        binder_->Draw("WindMap", "風のノイズテクスチャ");
        ImGui::Separator();
        binder_->Draw("WindSpeedMult", "突風の模様が流れる速さの倍率");

        ImGui::Separator();
        binder_->Draw("GustScale", "突風ノイズのスケール");
        binder_->Draw("GustStrength", "突風の強さ");
        binder_->Draw("WindFlattenStrength", "風で下へ倒れる割合");
        binder_->Draw("FlutterAmount", "葉先の震えの強さ");
    }

    if (ImGui::CollapsingHeader("インタラクション"))
    {
        binder_->Draw("InteractStrength", "押し倒す全体の強さ");
        binder_->Draw("FlattenFactor", "地面への押し潰し強度 (Y軸)");
        binder_->Draw("TrailFlattenWeight", "足跡痕跡による倒れ維持率");
    }

    if (ImGui::CollapsingHeader("影の設定"))
    {
        binder_->Draw("ShadowDensity", "影の濃さ");
        binder_->Draw("ShadowBias", "深度バイアス");
        binder_->Draw("ShadowNormalBias", "法線バイアス");
    }

    if (ImGui::CollapsingHeader("カリング・LOD設定"))
    {
        ImGui::Text("カメラ近接フェード");
        binder_->Draw("NearFadeMinDist", "完全透明距離 (Min)");
        binder_->Draw("NearFadeMaxDist", "フェード開始距離 (Max)");

        ImGui::Separator();
        binder_->Draw("MaxDrawDistance", "最大描画距離");

        ImGui::Separator();
        binder_->Draw("ThinStartDistance", "間引き開始距離");
        binder_->Draw("MaxThinningRate", "最大間引き率 (1.0に近いほど消える)");
        binder_->Draw("MaxWidthMultiplier", "間引き時の太さ補正倍率");

        ImGui::Separator();
        binder_->Draw("LodDistance1", "LOD1の距離 (高ポリ境界)");
        binder_->Draw("LodDistance2", "LOD2の距離 (中ポリ境界)");
    }

    if (ImGui::CollapsingHeader("延焼"))
    {
        ImGui::TextDisabled("焼夷弾 (Q / Y で切り替え) で撃った所の草に火がつく");
        if (ImGui::Button("燃え跡を消す"))
        {
            engine_->GetRendererManager()->ResetGrassFire();
        }

        ImGui::SeparatorText("燃え広がり方");
        binder_->Draw("Fire_BurnDuration", "1 か所が燃え尽きるまで (秒)");
        binder_->Draw("Fire_SpreadRate", "燃え移る速さ");
        binder_->Draw("Fire_MaxSpreadDistance", "火をつけた所から燃え広がる距離 (m、0 で止めない)");
        binder_->Draw("Fire_WindSpreadBias", "風下への燃え広がりやすさ");
        binder_->Draw("Fire_RainSuppression", "雨で燃え広がりにくくなる割合");
        binder_->Draw("Fire_RainSpreadDistanceRate", "濡れきったときの燃え広がる距離 (晴れの何倍か)");
        binder_->Draw("Fire_CoolingRate", "火が来なくなった所が冷める速さ");

        ImGui::SeparatorText("見た目");
        binder_->Draw("Fire_CharHeightRatio", "燃え尽きた草の高さ (元に対する割合)");
        binder_->Draw("Fire_ScorchColor", "熱で枯れた草の色");
        binder_->Draw("Fire_CharColor", "炭の色");
        binder_->Draw("Fire_GroundScorchDarkness", "焼け跡の地面の暗さ");
    }

    ImGui::End();
}

void GrassField::GenerateGrass()
{
    needsRegenerate_ = false;
    generatedTerrainVersion_ = terrain_->GetShapeVersion();

    const Vector2 areaSize = terrain_->GetSizeXZ();
    GrassGenerationData genData{};

    // 地形全体を敷き詰めるのに必要な本数
    uint32_t gridX = static_cast<uint32_t>(std::ceil(areaSize.x / gridSpacing_));
    uint32_t gridZ = static_cast<uint32_t>(std::ceil(areaSize.y / gridSpacing_));
    uint32_t neededGrassCount = gridX * gridZ;

    // 設定した上限と必要数のうち、小さい方を採用
    genData.maxGrassCount = std::min(neededGrassCount, static_cast<uint32_t>(maxGrassCount_));
    genData.gridSpacing = gridSpacing_;

    // スケールを掛け合わせて最終的な高さを決定
    genData.minHeight = minHeight_ * baseScale_;
    genData.maxHeight = maxHeight_ * baseScale_;
    genData.minWidth = minWidth_ * baseScale_;
    genData.maxWidth = maxWidth_ * baseScale_;
    genData.densityThreshold = densityThreshold_;

    genData.terrainCenter = terrain_->GetCenterXZ();
    genData.terrainWidth = areaSize.x;
    genData.terrainDepth = areaSize.y;

    grassSystem_->Generate(genData, terrain_->GetHeightmapName(), densityMapName_);

    generatedData_ = genData;
    LoadDensityMapForQuery();
}

void GrassField::LoadDensityMapForQuery()
{
    queryDensities_.clear();
    queryWidth_ = 0;
    queryHeight_ = 0;

    const FS::TextureHandleData* metaData = FS::TextureManager::GetInstance().GetMetaData(densityMapName_);
    if (!metaData)
    {
        return;
    }

    const std::wstring path = FS::StringUtils::ConvertString(metaData->fullPath);
    const bool isDDS = std::filesystem::path(metaData->fullPath).extension() == ".dds";

    DirectX::ScratchImage image;
    HRESULT result = isDDS
        ? DirectX::LoadFromDDSFile(path.c_str(), DirectX::DDS_FLAGS_NONE, nullptr, image)
        : DirectX::LoadFromWICFile(path.c_str(), DirectX::WIC_FLAGS_NONE, nullptr, image);
    if (FAILED(result))
    {
        return;
    }

    const DXGI_FORMAT format = image.GetMetadata().format;
    if (format != DXGI_FORMAT_R32G32B32A32_FLOAT)
    {
        DirectX::ScratchImage converted;
        result = DirectX::IsCompressed(format)
            ? DirectX::Decompress(*image.GetImage(0, 0, 0), DXGI_FORMAT_R32G32B32A32_FLOAT, converted)
            : DirectX::Convert(*image.GetImage(0, 0, 0), DXGI_FORMAT_R32G32B32A32_FLOAT, DirectX::TEX_FILTER_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT, converted);
        if (FAILED(result))
        {
            return;
        }
        image = std::move(converted);
    }

    const DirectX::Image* pixels = image.GetImage(0, 0, 0);
    queryWidth_ = static_cast<uint32_t>(pixels->width);
    queryHeight_ = static_cast<uint32_t>(pixels->height);
    queryDensities_.resize(static_cast<size_t>(queryWidth_) * queryHeight_);

    for (uint32_t y = 0; y < queryHeight_; ++y)
    {
        const float* row = reinterpret_cast<const float*>(pixels->pixels + pixels->rowPitch * y);
        for (uint32_t x = 0; x < queryWidth_; ++x)
        {
            queryDensities_[static_cast<size_t>(y) * queryWidth_ + x] = row[x * 4];
        }
    }
}

float GrassField::SampleQueryDensity(float u, float v) const
{
    // 画素の中心を基準に 4 画素を混ぜる
    const float pixelX = std::clamp(u * queryWidth_ - 0.5f, 0.0f, static_cast<float>(queryWidth_ - 1));
    const float pixelY = std::clamp(v * queryHeight_ - 0.5f, 0.0f, static_cast<float>(queryHeight_ - 1));
    const uint32_t x0 = static_cast<uint32_t>(pixelX);
    const uint32_t y0 = static_cast<uint32_t>(pixelY);
    const uint32_t x1 = std::min(x0 + 1, queryWidth_ - 1);
    const uint32_t y1 = std::min(y0 + 1, queryHeight_ - 1);
    const float blendX = pixelX - static_cast<float>(x0);
    const float blendY = pixelY - static_cast<float>(y0);

    auto At = [&](uint32_t x, uint32_t y) { return queryDensities_[static_cast<size_t>(y) * queryWidth_ + x]; };
    const float top = At(x0, y0) + (At(x1, y0) - At(x0, y0)) * blendX;
    const float bottom = At(x0, y1) + (At(x1, y1) - At(x0, y1)) * blendX;
    return top + (bottom - top) * blendY;
}

bool GrassField::IsGrassAt(float worldX, float worldZ) const
{
    if (queryDensities_.empty() || generatedData_.gridSpacing <= 0.0f)
    {
        return false;
    }

    const float spacing = generatedData_.gridSpacing;
    const float areaWidth = generatedData_.terrainWidth;
    const float areaDepth = generatedData_.terrainDepth;
    const Vector2& center = generatedData_.terrainCenter;

    // 本数の上限で生やせなかったマスには草がないので、生成と同じ格子の番号で調べる
    const uint32_t gridCountX = std::max(static_cast<uint32_t>(std::ceil(areaWidth / spacing)), 1u);
    const float originX = std::floor(center.x / spacing) * spacing - areaWidth * 0.5f;
    const float originZ = std::floor(center.y / spacing) * spacing - areaDepth * 0.5f;
    const float column = std::round((worldX - originX) / spacing);
    const float row = std::round((worldZ - originZ) / spacing);
    if (column < 0.0f || row < 0.0f || column >= static_cast<float>(gridCountX))
    {
        return false;
    }
    const uint64_t instanceIndex = static_cast<uint64_t>(row) * gridCountX + static_cast<uint64_t>(column);
    if (instanceIndex >= generatedData_.maxGrassCount)
    {
        return false;
    }

    const float u = (worldX - center.x) / areaWidth + 0.5f;
    const float v = (worldZ - center.y) / areaDepth + 0.5f;
    if (u < 0.0f || u > 1.0f || v < 0.0f || v > 1.0f)
    {
        return false;
    }

    return SampleQueryDensity(u, v) >= generatedData_.densityThreshold;
}