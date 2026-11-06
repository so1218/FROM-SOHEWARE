#include "pch.h"
#include "TreeField.h"
#include "ImGuiManager.h"
#include "MathUtils.h"
#include "Model.h"

using namespace FS;

namespace
{
    // 毎回同じ配置になるように固定
    constexpr uint32_t kPlacementSeed = 1337;
    // 弾が当たる高さ
    constexpr float kHitHeightRate = 1.5f;
}

TreeField::TreeField(Engine* engine) : GameObject()
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
                toonRampName_ = trunkHandle->toonRampName;
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
    binder_->BindColor("TrunkSpecColor", &trunkMaterial_.specularColor, { 1.0f, 1.0f, 1.0f, 1.0f });
    binder_->Bind("TrunkRoughness", &trunkMaterial_.roughness, 0.8f, 0.01f, 0.0f, 1.0f);
    binder_->Bind("TrunkMetalness", &trunkMaterial_.metalness, 0.0f, 0.01f, 0.0f, 1.0f);
    binder_->Bind("TrunkShininess", &trunkMaterial_.shininess, 10.0f, 0.1f, 0.0f, 100.0f);
    binder_->Bind("TrunkDiffReflect", &trunkMaterial_.diffuseReflection, 1.0f, 0.01f, 0.0f, 5.0f);
    binder_->Bind("TrunkEnvMapInt", &trunkMaterial_.environmentMapIntensity, 1.0f, 0.01f, 0.0f, 5.0f);
    binder_->Bind("TrunkNormalInt", &trunkMaterial_.normalIntensity, 1.0f, 0.01f, 0.0f, 5.0f);
    binder_->Bind("TrunkShadowDens", &trunkMaterial_.shadowDensity, 0.8f, 0.01f, 0.0f, 1.0f);
    binder_->Bind("TrunkShadowBias", &trunkMaterial_.shadowBias, 0.005f, 0.0001f, 0.0f, 0.1f);
    binder_->Bind("TrunkShadowNBias", &trunkMaterial_.shadowNormalBias, 0.02f, 0.001f, 0.0f, 0.5f);
    binder_->Bind("TrunkShadowSoft", &trunkMaterial_.shadowSoftness, 1.0f, 0.01f, 0.0f, 5.0f);
    binder_->Bind("TrunkShadowEnvStr", &trunkMaterial_.shadowEnvStrength, 0.5f, 0.01f, 0.0f, 1.0f);

    // 幹は常にライティング・法線マップ・影ありで描く
    trunkMaterial_.enableLighting = 1;
    trunkMaterial_.lightMode = 1;
    trunkMaterial_.enableNormalMap = 1;
    trunkMaterial_.addShadow = 1;

    auto onTextureChanged = [this]() { ApplyTextures(); };
    binder_->BindTexture("WindMap", &windMapName_, &windMapHandle_, "noise_39", TextureType::Noise,
        [this]() { treeSystem_->SetWindMapTexture(windMapName_); });
    binder_->BindTexture("LeafTexture", &leafTextureName_, &leafTextureHandle_, leafTextureName_, TextureType::Albedo, onTextureChanged);
    binder_->BindTexture("TrunkTexture", &trunkTextureName_, &trunkTextureHandle_, trunkTextureName_, TextureType::Albedo, onTextureChanged);
    binder_->BindTexture("TrunkNormal", &trunkNormalName_, &trunkNormalHandle_, trunkNormalName_, TextureType::Normal, onTextureChanged);
    binder_->BindTexture("ToonRamp", &toonRampName_, &toonRampHandle_, toonRampName_, TextureType::Toon, onTextureChanged);

    treeSystem_->SetWindMapTexture(windMapName_);
    GenerateTrees();
    ApplyMaterials();
}

void TreeField::Update()
{
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
    treeMaterialHandle_.toonRampHandle = toonRampHandle_;
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
        treeMaterialHandle_.toonRampHandle = toonRampHandle_;
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
        isMaterialChanged |= binder_->Draw("TrunkSpecColor", "スペキュラカラー");
        isMaterialChanged |= binder_->Draw("TrunkRoughness", "ラフネス");
        isMaterialChanged |= binder_->Draw("TrunkMetalness", "メタルネス");
        isMaterialChanged |= binder_->Draw("TrunkShininess", "ハイライトの鋭さ");
        isMaterialChanged |= binder_->Draw("TrunkDiffReflect", "ディフューズ反射");
        isMaterialChanged |= binder_->Draw("TrunkNormalInt", "ノーマルマップ強度");
        isMaterialChanged |= binder_->Draw("TrunkEnvMapInt", "環境マップ反射強度");

        ImGui::SeparatorText("影");
        isMaterialChanged |= binder_->Draw("TrunkShadowDens", "影の濃さ");
        isMaterialChanged |= binder_->Draw("TrunkShadowBias", "シャドウ深度バイアス");
        isMaterialChanged |= binder_->Draw("TrunkShadowNBias", "シャドウノーマルバイアス");
        isMaterialChanged |= binder_->Draw("TrunkShadowSoft", "影のソフトネス");
        isMaterialChanged |= binder_->Draw("TrunkShadowEnvStr", "環境マップの影への影響");
    }

    if (ImGui::CollapsingHeader("共通"))
    {
        binder_->Draw("ToonRamp", "トゥーンランプ");
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
