#include "pch.h"
#include "FoliageSystem.h"
#include "Engine.h"
#include "TextureManager.h"

namespace FE
{

FoliageSystem::FoliageSystem(Engine* engine) : engine_(engine)
{}

void FoliageSystem::AddFoliageType(
    const std::string& albedoTextureName,
    const std::string& normalTextureName,
    const std::string& densityMapName,
    const MeshData& meshData,
    const FoliageMaterialData& defaultMaterial,
    const FoliageGenerationData& defaultGenData)
{
    FoliageTypeInfo info;
    info.albedoSrvHandle = TextureManager::GetInstance().Get(albedoTextureName);
    info.normalSrvHandle = TextureManager::GetInstance().Get(normalTextureName);
    info.densityMapSrvHandle = TextureManager::GetInstance().Get(densityMapName);

    // メッシュの初期化
    auto* device = engine_->GetGraphicsDevice()->GetDevice();
    info.mesh = std::make_unique<Mesh>();
    info.mesh->Initialize(device, meshData.vertices, meshData.indices);
    info.mesh->SetVertexCount(static_cast<uint32_t>(meshData.vertices.size()));
    info.mesh->SetIndexCount(static_cast<uint32_t>(meshData.indices.size()));

    info.material = defaultMaterial;
    info.genData = defaultGenData;

    foliageTypes_.push_back(std::move(info));
}

void FoliageSystem::InitializeRenderer()
{
    if (!engine_ || foliageTypes_.empty()) return;

    // RendererManager に渡すための Config 配列を構築
    std::vector<FoliageTypeConfig> configs;
    configs.reserve(foliageTypes_.size());

    for (const auto& typeInfo : foliageTypes_)
    {
        FoliageTypeConfig config;
        config.mesh = typeInfo.mesh.get(); // ポインタを渡す
        config.albedoSrvHandle = typeInfo.albedoSrvHandle;
        config.normalSrvHandle = typeInfo.normalSrvHandle;
        config.densityMapSrvHandle = typeInfo.densityMapSrvHandle;
        config.material = typeInfo.material;
        config.genData = typeInfo.genData;
        configs.push_back(config);
    }

    auto* rendererManager = engine_->GetRendererManager();
    // ★ RendererManager に追加した初期化関数を呼ぶ
    rendererManager->InitializeFoliage(configs);

    isRendererInitialized_ = true;
}

void FoliageSystem::Generate(
    const std::string& heightMapName,
    UINT terrainWidth, UINT terrainDepth)
{
    if (!engine_ || !isRendererInitialized_) return;

    auto* rendererManager = engine_->GetRendererManager();

    uint32_t heightMapHandle = TextureManager::GetInstance().Get(heightMapName);

    rendererManager->GenerateFoliage(heightMapHandle, terrainWidth, terrainDepth);
}

void FoliageSystem::Update()
{
    if (!engine_ || !isRendererInitialized_) return;

    auto* rendererManager = engine_->GetRendererManager();

    // 毎フレームカリング情報を更新
    rendererManager->SetFoliageRenderingParams(cullingData_);

}

void FoliageSystem::UpdateConfigs(const std::vector<FoliageLayer>& layers)
{
    if (!isRendererInitialized_) return;

    std::vector<FoliageTypeConfig> configs;
    configs.reserve(foliageTypes_.size());

    // UIで変更された最新のテクスチャ名から、ハンドルを取り直してConfigを作る
    for (size_t i = 0; i < foliageTypes_.size(); ++i)
    {
        const auto& layer = layers[i];
        auto& typeInfo = foliageTypes_[i];

        // 最新のハンドルを取得
        typeInfo.albedoSrvHandle = TextureManager::GetInstance().Get(layer.albedoName);
        typeInfo.normalSrvHandle = TextureManager::GetInstance().Get(layer.normalName);
        typeInfo.densityMapSrvHandle = TextureManager::GetInstance().Get(layer.densityMapName);

        // ※必要であればメッシュの更新処理もここに入れます

        FoliageTypeConfig config;
        config.mesh = typeInfo.mesh.get();
        config.albedoSrvHandle = typeInfo.albedoSrvHandle;
        config.normalSrvHandle = typeInfo.normalSrvHandle;
        config.densityMapSrvHandle = typeInfo.densityMapSrvHandle;
        config.material = layer.material;
        config.genData = layer.genData;

        configs.push_back(config);
    }

    // レンダラーマネージャー経由で Renderer に新しい設定を送る
    engine_->GetRendererManager()->UpdateFoliageConfigs(configs);
}

FoliageMaterialData* FoliageSystem::GetMaterialData(size_t index)
{
    if (index < foliageTypes_.size()) return &foliageTypes_[index].material;
    return nullptr;
}

FoliageGenerationData* FoliageSystem::GetGenerationData(size_t index)
{
    if (index < foliageTypes_.size()) return &foliageTypes_[index].genData;
    return nullptr;
}

}