#include "pch.h"
#include "TreeField.h"
#include "ImGuiManager.h"
#include "MathUtils.h"
#include "Model.h"
#include "DistantMountains.h"

using namespace FS;

namespace
{
    // 毎回同じ配置になるように固定
    constexpr uint32_t kPlacementSeed = 1337;
    // 弾が当たる高さ
    constexpr float kHitHeightRate = 1.5f;
}

TreeField::TreeField(Engine* engine, Terrain* terrain) : GameObject(), terrain_(terrain)
{
    engine_ = engine;

    model_ = std::make_unique<Model>(engine_, "tree");
    treeSystem_ = std::make_unique<TreeSystem>(engine_, "noise_39");
    binder_ = std::make_unique<PropertyBinder>(engine_, "TreeField");
}

void TreeField::Initialize()
{
    // メッシュ 0 が幹、1 が葉
    if (model_)
    {
        if (model_->GetMaterialCount() > 0)
        {
            if (auto* trunkHandle = model_->GetMaterialHandle(0))
            {
                trunkTextureName_ = trunkHandle->textureName;
                trunkNormalName_ = trunkHandle->normalMapName;
            }
        }
        if (model_->GetMaterialCount() > 1)
        {
            if (auto* leafHandle = model_->GetMaterialHandle(1))
            {
                leafTextureName_ = leafHandle->textureName;
            }
        }
    }

    binder_->Bind("TreeCount", &treeCount_, 500, 1, 10, 5000);
    binder_->Bind("AreaWidth", &areaWidth_, 400.0f, 1.0f, 10.0f, 2000.0f);
    binder_->Bind("AreaDepth", &areaDepth_, 400.0f, 1.0f, 10.0f, 2000.0f);
    binder_->Bind("AreaCenterX", &areaCenter_.x, 0.0f, 1.0f, -2000.0f, 2000.0f);
    binder_->Bind("AreaCenterZ", &areaCenter_.y, 0.0f, 1.0f, -2000.0f, 2000.0f);
    binder_->Bind("MinScale", &minScale_, 0.8f, 0.05f, 0.1f);
    binder_->Bind("MaxScale", &maxScale_, 1.4f, 0.05f, 0.1f);
    binder_->Bind("TrunkColliderRadius", &trunkColliderRadius_, 0.5f, 0.05f, 0.05f, 5.0f);
    binder_->Bind("MaxDrawDistance", &maxDrawDistance_, 300.0f, 5.0f, 50.0f, 2000.0f);
    binder_->Bind("TreeRadius", &treeRadius_, 5.0f, 0.1f, 0.1f, 20.0f);

    binder_->Bind("BoundaryForestEnable", &isBoundaryForestEnabled_, true);
    binder_->Bind("BoundaryInnerTreeCount", &boundaryInnerTreeCount_, 500, 1.0f, 0, 3500);
    binder_->Bind("BoundaryOuterTreeCount", &boundaryOuterTreeCount_, 1000, 1.0f, 0, 3500);
    binder_->Bind("BoundaryInnerWidth", &boundaryInnerWidth_, 20.0f, 0.5f, 0.0f, 200.0f);
    binder_->Bind("BoundaryOuterWidth", &boundaryOuterWidth_, 150.0f, 1.0f, 0.0f, 1000.0f);

    // 葉
    binder_->BindColor("LeafColorTint", &leafMaterial_.colorTint, { 1.0f, 1.0f, 1.0f });
    binder_->Bind("TreeHeight", &leafMaterial_.treeHeight, 10.0f, 0.1f, 1.0f, 50.0f);
    binder_->Bind("WindSpeedMult", &leafMaterial_.windSpeedMultiplier, 1.0f, 0.01f, 0.0f, 5.0f);
    binder_->Bind("GustScale", &leafMaterial_.gustScale, 0.05f, 0.01f, 0.0f, 1.0f);
    binder_->Bind("GustStrength", &leafMaterial_.gustStrength, 1.0f, 0.01f, 0.0f, 5.0f);
    binder_->Bind("TrunkFlex", &leafMaterial_.trunkFlexibility, 0.1f, 0.01f, 0.0f, 2.0f);
    binder_->Bind("LeafFlutter", &leafMaterial_.leafFlutterAmount, 0.2f, 0.01f, 0.0f, 2.0f);
    binder_->Bind("DiffuseWrap", &leafMaterial_.diffuseWrap, 0.2f, 0.01f, 0.0f, 1.0f);
    binder_->Bind("SSSStrength", &leafMaterial_.sssStrength, 1.0f, 0.01f, 0.0f, 5.0f);
    binder_->BindColor("SSSColor", &leafMaterial_.sssColor, { 0.5f, 0.7f, 0.2f });
    binder_->Bind("AlphaCutoff", &leafMaterial_.alphaCutoff, 0.5f, 0.01f, 0.0f, 1.0f);
    binder_->Bind("BaseRoughness", &leafMaterial_.baseRoughness, 0.5f, 0.01f, 0.0f, 1.0f);
    binder_->Bind("BaseAO", &leafMaterial_.baseAO, 1.0f, 0.01f, 0.0f, 2.0f);
    binder_->Bind("LeafShadowDensity", &leafMaterial_.shadowDensity, 0.8f, 0.01f, 0.0f, 1.0f);
    binder_->Bind("LeafShadowNormBias", &leafMaterial_.shadowNormalBias, 0.02f, 0.001f, 0.0f, 0.5f);
    binder_->Bind("LeafShadowBias", &leafMaterial_.shadowBias, 0.005f, 0.0001f, 0.0f, 0.1f);
    binder_->Bind("NearFadeMinDist", &leafMaterial_.nearFadeMinDist, 1.0f, 0.1f, 0.0f, 10.0f);
    binder_->Bind("NearFadeMaxDist", &leafMaterial_.nearFadeMaxDist, 3.0f, 0.1f, 0.1f, 20.0f);

    // 幹
    binder_->BindColor("TrunkColor", &trunkMaterial_.color, { 1.0f, 1.0f, 1.0f, 1.0f });
    binder_->Bind("TrunkRoughness", &trunkMaterial_.roughness, 0.8f, 0.01f, 0.0f, 1.0f);
    binder_->Bind("TrunkEnvMapInt", &trunkMaterial_.environmentMapIntensity, 1.0f, 0.01f, 0.0f, 5.0f);
    binder_->Bind("TrunkNormalInt", &trunkMaterial_.normalIntensity, 1.0f, 0.01f, 0.0f, 5.0f);
    binder_->Bind("TrunkShadowDens", &trunkMaterial_.shadowDensity, 0.8f, 0.01f, 0.0f, 1.0f);
    binder_->Bind("TrunkShadowBias", &trunkMaterial_.shadowBias, 0.005f, 0.0001f, 0.0f, 0.1f);
    binder_->Bind("TrunkShadowNBias", &trunkMaterial_.shadowNormalBias, 0.02f, 0.001f, 0.0f, 0.5f);
    binder_->Bind("TrunkShadowSoft", &trunkMaterial_.shadowSoftness, 1.0f, 0.01f, 0.0f, 5.0f);

    auto onTextureChanged = [this]() { ApplyTextures(); };
    binder_->BindTexture("WindMap", &windMapName_, &windMapHandle_, "noise_39", TextureType::Noise,
        [this]() { treeSystem_->SetWindMapTexture(windMapName_); });
    binder_->BindTexture("LeafTexture", &leafTextureName_, &leafTextureHandle_, leafTextureName_, TextureType::Albedo, onTextureChanged);
    binder_->BindTexture("TrunkTexture", &trunkTextureName_, &trunkTextureHandle_, trunkTextureName_, TextureType::Albedo, onTextureChanged);
    binder_->BindTexture("TrunkNormal", &trunkNormalName_, &trunkNormalHandle_, trunkNormalName_, TextureType::Normal, onTextureChanged);

    treeSystem_->SetWindMapTexture(windMapName_);
    GenerateTrees();
    ApplyMaterials();
}

void TreeField::Update()
{
    // 遠景の山が作り直されたら、ふもとの木の高さを合わせ直す
    if (distantMountains_ && distantMountains_->GetShapeVersion() != placedMountainVersion_)
    {
        GenerateTrees();
    }

    treeSystem_->Update();
}

void TreeField::ApplyMaterials()
{
    if (!treeMaterialHandle_.leafMaterialBuffer) return;

    trunkMaterial_.nearFadeMinDist = leafMaterial_.nearFadeMinDist;
    trunkMaterial_.nearFadeMaxDist = leafMaterial_.nearFadeMaxDist;

    treeSystem_->UpdateLeafMaterial(treeMaterialHandle_, leafMaterial_);
    treeSystem_->UpdateTrunkMaterial(treeMaterialHandle_, trunkMaterial_);
    treeSystem_->SetCullingParameters(maxDrawDistance_, leafMaterial_.treeHeight, treeRadius_);
}

void TreeField::ApplyTextures()
{
    if (!treeMaterialHandle_.leafMaterialBuffer) return;

    treeMaterialHandle_.leafTextureHandle = leafTextureHandle_;
    treeMaterialHandle_.trunkTextureHandle = trunkTextureHandle_;
    treeMaterialHandle_.trunkNormalMapHandle = trunkNormalHandle_;
}

void TreeField::GenerateTrees()
{
    treeSystem_->Clear();
    colliders_.clear();

    if (!model_) return;

    const ModelData* modelData = model_->GetModelData();
    if (!modelData) return;

    // マテリアルは全部の木で 1 つを共有
    if (!treeMaterialHandle_.leafMaterialBuffer)
    {
        treeMaterialHandle_ = treeSystem_->CreateTreeMaterial(
            leafMaterial_, trunkMaterial_, leafTextureHandle_, trunkTextureHandle_, trunkNormalHandle_);
    }

    std::mt19937 random(kPlacementSeed);
    std::uniform_real_distribution<float> offsetX(-areaWidth_ * 0.5f, areaWidth_ * 0.5f);
    std::uniform_real_distribution<float> offsetZ(-areaDepth_ * 0.5f, areaDepth_ * 0.5f);
    std::uniform_real_distribution<float> scaleRange(minScale_, maxScale_);
    std::uniform_real_distribution<float> rotationRange(0.0f, 360.0f);

    for (int treeIndex = 0; treeIndex < treeCount_; ++treeIndex)
    {
        Vector3 position = { areaCenter_.x + offsetX(random), 0.0f, areaCenter_.y + offsetZ(random) };
        if (terrain_)
        {
            position.y = terrain_->GetHeight(position.x, position.z);
        }

        const float scale = scaleRange(random);
        const float rotationY = rotationRange(random);

        WorldTransform treeTransform(Vector3(scale, scale, scale), Vector3(0.0f, rotationY, 0.0f), position);
        treeSystem_->AddInstance(treeTransform, *modelData, treeMaterialHandle_);

        colliders_.push_back({ position, trunkColliderRadius_ * scale });
    }

    if (isBoundaryForestEnabled_)
    {
        AddBoundaryTrees(*modelData);
    }
}

void TreeField::AddBoundaryTrees(const ModelData& modelData)
{
    if (!terrain_) return;

    const Vector2 size = terrain_->GetSizeXZ();
    if (size.x <= 0.0f || size.y <= 0.0f) return;

    const Vector2 center = terrain_->GetCenterXZ();
    const Vector2 areaMin = center - size * 0.5f;
    const Vector2 areaMax = center + size * 0.5f;

    if (distantMountains_)
    {
        placedMountainVersion_ = distantMountains_->GetShapeVersion();
    }
    // 遠景の山がないときは、外に置くと宙に浮くので内側だけにする
    const float outerWidth = distantMountains_ ? boundaryOuterWidth_ : 0.0f;

    // 端の木の並びが変わらないよう、ほかの木とは別の乱数を使う
    std::mt19937 random(kPlacementSeed + 1);
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);
    std::uniform_real_distribution<float> scaleRange(minScale_, maxScale_);
    std::uniform_real_distribution<float> rotationRange(0.0f, 360.0f);

    // 内側は同じ密度、外側は端から離れるほど直線的にまばらにする
    const int outerTreeCount = distantMountains_ ? boundaryOuterTreeCount_ : 0;
    const int totalTreeCount = boundaryInnerTreeCount_ + outerTreeCount;

    const float perimeter = (size.x + size.y) * 2.0f;
    for (int treeIndex = 0; treeIndex < totalTreeCount; ++treeIndex)
    {
        // 外向きを正とした、端からの距離
        float offset = 0.0f;
        if (treeIndex < boundaryInnerTreeCount_)
        {
            offset = -boundaryInnerWidth_ * unit(random);
        }
        else
        {
            offset = outerWidth * (1.0f - std::sqrt(1.0f - unit(random)));
        }

        // 周の長さで辺を選ぶ。外側の木は辺の両端より先にもはみ出させ、角の外が空かないようにする
        float along = unit(random) * perimeter;
        const float overhang = std::max(offset, 0.0f);
        const auto AlongSide = [&](float sideLength) { return -overhang + (along / sideLength) * (sideLength + overhang * 2.0f); };

        Vector3 position = { 0.0f, 0.0f, 0.0f };
        if (along < size.x)
        {
            position = { areaMin.x + AlongSide(size.x), 0.0f, areaMin.y - offset };
        }
        else if ((along -= size.x) < size.y)
        {
            position = { areaMax.x + offset, 0.0f, areaMin.y + AlongSide(size.y) };
        }
        else if ((along -= size.y) < size.x)
        {
            position = { areaMax.x - AlongSide(size.x), 0.0f, areaMax.y + offset };
        }
        else
        {
            along -= size.x;
            position = { areaMin.x - offset, 0.0f, areaMax.y - AlongSide(size.y) };
        }

        const bool isInside = position.x >= areaMin.x && position.x < areaMax.x && position.z >= areaMin.y && position.z < areaMax.y;
        if (isInside)
        {
            position.y = terrain_->GetHeight(position.x, position.z);
        }
        else if (distantMountains_)
        {
            position.y = distantMountains_->GetHeight(position.x, position.z);
        }
        else
        {
            continue;
        }

        const float scale = scaleRange(random);
        const float rotationY = rotationRange(random);

        WorldTransform treeTransform(Vector3(scale, scale, scale), Vector3(0.0f, rotationY, 0.0f), position);
        treeSystem_->AddInstance(treeTransform, modelData, treeMaterialHandle_);

        // 外の木にはプレイヤーは届かないが、弾は当たるようにする
        colliders_.push_back({ position, trunkColliderRadius_ * scale });
    }
}

bool TreeField::ResolveCollision(Vector3& playerPos, float playerRadius) const
{
    bool isHit = false;

    for (const TreeCollider& tree : colliders_)
    {
        // 幹は縦に長いので XZ 平面の円で押し戻す
        const float offsetX = playerPos.x - tree.position.x;
        const float offsetZ = playerPos.z - tree.position.z;
        const float distanceSq = offsetX * offsetX + offsetZ * offsetZ;
        const float minDistance = playerRadius + tree.radius;

        if (distanceSq < minDistance * minDistance && distanceSq > 0.00001f)
        {
            const float distance = std::sqrt(distanceSq);
            const float overlap = minDistance - distance;
            playerPos.x += offsetX / distance * overlap;
            playerPos.z += offsetZ / distance * overlap;
            isHit = true;
        }
    }

    return isHit;
}

bool TreeField::Raycast(const Vector3& rayStart, const Vector3& rayDir, float maxDistance, TreeRaycastHit* outHit) const
{
    bool isHit = false;
    float closestDistance = maxDistance;
    TreeRaycastHit closestHit{};

    const Vector2 rayStartXZ = { rayStart.x, rayStart.z };
    const Vector2 rayDirXZ = { rayDir.x, rayDir.z };
    const float hitHeight = leafMaterial_.treeHeight * kHitHeightRate;

    for (const TreeCollider& tree : colliders_)
    {
        // XZ 平面でレイと幹の円が交わるか
        const Vector2 fromCenter = rayStartXZ - Vector2(tree.position.x, tree.position.z);
        const float a = rayDirXZ.Dot(rayDirXZ);
        const float b = 2.0f * fromCenter.Dot(rayDirXZ);
        const float c = fromCenter.Dot(fromCenter) - tree.radius * tree.radius;
        const float discriminant = b * b - 4.0f * a * c;
        if (discriminant < 0.0f) continue;

        // 手前側の交点
        const float distance = (-b - std::sqrt(discriminant)) / (2.0f * a);
        if (distance <= 0.0f || distance >= closestDistance) continue;

        const Vector3 hitPoint = rayStart + rayDir * distance;
        if (hitPoint.y < tree.position.y || hitPoint.y > tree.position.y + hitHeight) continue;

        const Vector3 normalXZ = Vector3(hitPoint.x - tree.position.x, 0.0f, hitPoint.z - tree.position.z);
        closestDistance = distance;
        closestHit.point = hitPoint;
        closestHit.distance = distance;
        closestHit.normal = normalXZ.LengthSq() > 0.0001f ? normalXZ.Normalize() : Vector3(0.0f, 1.0f, 0.0f);
        isHit = true;
    }

    if (isHit && outHit)
    {
        *outHit = closestHit;
    }
    return isHit;
}

void TreeField::DebugGui()
{
    ImGui::Begin("森林");

    // 配置を変えたら木を作り直し、マテリアルを変えたら GPU に送り直す
    bool isPlacementChanged = false;
    bool isMaterialChanged = false;

    if (ImGui::CollapsingHeader("配置 (変えると作り直す)", ImGuiTreeNodeFlags_DefaultOpen))
    {
        isPlacementChanged |= binder_->Draw("TreeCount", "木の本数");
        isPlacementChanged |= binder_->Draw("AreaWidth", "配置範囲 (幅 X)");
        isPlacementChanged |= binder_->Draw("AreaDepth", "配置範囲 (奥行 Z)");
        isPlacementChanged |= binder_->Draw("AreaCenterX", "配置の中心 X");
        isPlacementChanged |= binder_->Draw("AreaCenterZ", "配置の中心 Z");
        isPlacementChanged |= binder_->Draw("MinScale", "最小スケール");
        isPlacementChanged |= binder_->Draw("MaxScale", "最大スケール");
        isPlacementChanged |= binder_->Draw("TrunkColliderRadius", "幹の当たり判定の半径 (スケール 1 のとき)");

        ImGui::SeparatorText("境界の森");
        isPlacementChanged |= binder_->Draw("BoundaryForestEnable", "置く");
        isPlacementChanged |= binder_->Draw("BoundaryInnerTreeCount", "内側 (歩ける地形の中) の本数");
        isPlacementChanged |= binder_->Draw("BoundaryOuterTreeCount", "外側 (遠景の山のふもと) の本数");
        isPlacementChanged |= binder_->Draw("BoundaryInnerWidth", "地形の端から内側へ置く幅");
        isPlacementChanged |= binder_->Draw("BoundaryOuterWidth", "地形の端から外側へ置く幅 (外ほどまばら)");

        ImGui::Separator();
        isMaterialChanged |= binder_->Draw("MaxDrawDistance", "最大描画距離");
        isMaterialChanged |= binder_->Draw("TreeRadius", "木の半径 (カリング用)");
        isMaterialChanged |= binder_->Draw("NearFadeMinDist", "カメラが近いとき完全に透ける距離");
        isMaterialChanged |= binder_->Draw("NearFadeMaxDist", "カメラが近いとき透け始める距離");
        binder_->Draw("WindMap", "風のノイズテクスチャ");
    }

    if (ImGui::CollapsingHeader("葉"))
    {
        binder_->Draw("LeafTexture", "アルベド");
        isMaterialChanged |= binder_->Draw("LeafColorTint", "色味");

        ImGui::SeparatorText("風・揺れ");
        isMaterialChanged |= binder_->Draw("TreeHeight", "木の高さ (基準)");
        isMaterialChanged |= binder_->Draw("WindSpeedMult", "揺れの速さ倍率 (環境の風に対して)");
        isMaterialChanged |= binder_->Draw("GustScale", "突風ノイズスケール");
        isMaterialChanged |= binder_->Draw("GustStrength", "突風の強さ");
        isMaterialChanged |= binder_->Draw("TrunkFlex", "幹のしなりやすさ");
        isMaterialChanged |= binder_->Draw("LeafFlutter", "葉の揺れ幅");

        ImGui::SeparatorText("質感・透過");
        isMaterialChanged |= binder_->Draw("DiffuseWrap", "ディフューズラップ");
        isMaterialChanged |= binder_->Draw("SSSStrength", "SSS 強度");
        isMaterialChanged |= binder_->Draw("SSSColor", "SSS カラー");
        isMaterialChanged |= binder_->Draw("AlphaCutoff", "アルファカットオフ");
        isMaterialChanged |= binder_->Draw("BaseRoughness", "基本ラフネス");
        isMaterialChanged |= binder_->Draw("BaseAO", "ベース AO");

        ImGui::SeparatorText("影");
        isMaterialChanged |= binder_->Draw("LeafShadowDensity", "影の濃さ");
        isMaterialChanged |= binder_->Draw("LeafShadowNormBias", "シャドウノーマルバイアス");
        isMaterialChanged |= binder_->Draw("LeafShadowBias", "シャドウ深度バイアス");
    }

    if (ImGui::CollapsingHeader("幹"))
    {
        binder_->Draw("TrunkTexture", "アルベド");
        binder_->Draw("TrunkNormal", "ノーマルマップ");

        ImGui::SeparatorText("質感");
        isMaterialChanged |= binder_->Draw("TrunkColor", "カラー");
        isMaterialChanged |= binder_->Draw("TrunkRoughness", "ラフネス");
        isMaterialChanged |= binder_->Draw("TrunkNormalInt", "ノーマルマップ強度");
        isMaterialChanged |= binder_->Draw("TrunkEnvMapInt", "環境マップ反射強度");

        ImGui::SeparatorText("影");
        isMaterialChanged |= binder_->Draw("TrunkShadowDens", "影の濃さ");
        isMaterialChanged |= binder_->Draw("TrunkShadowBias", "シャドウ深度バイアス");
        isMaterialChanged |= binder_->Draw("TrunkShadowNBias", "シャドウノーマルバイアス");
        isMaterialChanged |= binder_->Draw("TrunkShadowSoft", "影のソフトネス");
    }

    if (isPlacementChanged)
    {
        GenerateTrees();
    }
    if (isMaterialChanged)
    {
        ApplyMaterials();
    }

    ImGui::Separator();
    ImGui::Text("描画中の本数: %u", treeSystem_->GetInstanceCount());

    ImGui::End();
}
