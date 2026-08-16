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
    // アセットの追加は初期化フェーズに限定し、動的なバッファ再確保の複雑化を避ける
    assert(!isRendererInitialized_ && "Cannot add foliage types after renderer initialization.");

    FoliageTypeInfo info;
    info.albedoSrvHandle = TextureManager::GetInstance().Get(albedoTextureName);
    info.normalSrvHandle = TextureManager::GetInstance().Get(normalTextureName);
    info.densityMapSrvHandle = TextureManager::GetInstance().Get(densityMapName);

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

    std::vector<FoliageTypeConfig> configs;
    configs.reserve(foliageTypes_.size());

    // レンダラー側にはポインタ経由でメッシュやテクスチャハンドルを参照させ、
    // 実リソースの管理責任をSystem側に明確に分離する
    for (const auto& typeInfo : foliageTypes_)
    {
        FoliageTypeConfig config;
        config.mesh = typeInfo.mesh.get();
        config.albedoSrvHandle = typeInfo.albedoSrvHandle;
        config.normalSrvHandle = typeInfo.normalSrvHandle;
        config.densityMapSrvHandle = typeInfo.densityMapSrvHandle;
        config.material = typeInfo.material;
        config.genData = typeInfo.genData;
        configs.push_back(config);
    }

    engine_->GetRendererManager()->InitializeFoliage(configs);
    isRendererInitialized_ = true;
}

void FoliageSystem::Generate(
    const std::string& heightMapName,
    UINT terrainWidth, UINT terrainDepth)
{
    assert(isRendererInitialized_ && "Foliage renderer is not initialized.");
    if (!engine_ || !isRendererInitialized_) return;

    uint32_t heightMapHandle = TextureManager::GetInstance().Get(heightMapName);
    engine_->GetRendererManager()->GenerateFoliage(heightMapHandle, terrainWidth, terrainDepth);
}

void FoliageSystem::Update()
{
    if (!engine_ || !isRendererInitialized_) return;
    engine_->GetRendererManager()->SetFoliageRenderingParams(cullingData_);
}

void FoliageSystem::UpdateConfigs(const std::vector<FoliageLayer>& layers)
{
    if (!isRendererInitialized_) return;

    size_t count = std::min(foliageTypes_.size(), layers.size());
    std::vector<FoliageTypeConfig> configs;
    configs.reserve(count);

    // エディタ側でのテクスチャ差し替えや密度調整を即座に反映
    // メッシュトポロジの変更は伴わない前提のため、リソースの再構築はスキップ
    for (size_t i = 0; i < count; ++i)
    {
        const auto& layer = layers[i];
        auto& typeInfo = foliageTypes_[i];

        typeInfo.albedoSrvHandle = TextureManager::GetInstance().Get(layer.albedoName);
        typeInfo.normalSrvHandle = TextureManager::GetInstance().Get(layer.normalName);
        typeInfo.densityMapSrvHandle = TextureManager::GetInstance().Get(layer.densityMapName);

        FoliageTypeConfig config;
        config.mesh = typeInfo.mesh.get();
        config.albedoSrvHandle = typeInfo.albedoSrvHandle;
        config.normalSrvHandle = typeInfo.normalSrvHandle;
        config.densityMapSrvHandle = typeInfo.densityMapSrvHandle;
        config.material = layer.material;
        config.genData = layer.genData;

        configs.push_back(config);
    }

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