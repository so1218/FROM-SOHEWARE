#include "pch.h"
#include "TreeSystem.h"
#include "Engine.h"
#include "TextureManager.h"
#include "TreeRenderer.h"

namespace FE 
{

TreeSystem::TreeSystem(Engine* engine, const std::string& windMapTextureName)
    : engine_(engine)
{
    SetWindMapTexture(windMapTextureName);
}

void TreeSystem::SetWindMapTexture(const std::string& textureName)
{
    windMapTextureHandle_ = TextureManager::GetInstance().Get(textureName);
}

void TreeSystem::Clear()
{
    instances_.clear();
}

void TreeSystem::AddInstance(const TreeInstance& instance)
{
    instances_.push_back(instance);
}

void TreeSystem::AddInstance(
    const WorldTransform& transform,
    const ModelData& modelData,
    const TreeMaterialHandle& treeMaterial, 
    const Vector4& colorVariation,
    float lodFade)
{
    TreeInstance inst{};
    inst.transform = transform;
    inst.modelData = &modelData;
    inst.treeMaterial = treeMaterial;      
    inst.colorVariation = colorVariation;
    inst.lodFade = lodFade;
    instances_.push_back(inst);
}

void TreeSystem::Update()
{
    if (!engine_) return;

    auto* rendererManager = engine_->GetRendererManager();

    // 風テクスチャを RendererManager に登録
    rendererManager->SetWindMap(windMapTextureHandle_);

    // 登録されたすべての木を RendererManager に Submit
    for (const auto& inst : instances_)
    {
        if (inst.modelData)
        {
            rendererManager->SubmitTree(
                inst.transform,
                *inst.modelData,
                inst.treeMaterial,
                inst.colorVariation,
                inst.lodFade
            );
        }
    }
}

TreeMaterialHandle TreeSystem::CreateTreeMaterial(
    const LeafMaterialData& leafData,
    const TrunkMaterialData& trunkData,
    uint32_t leafTex, uint32_t trunkTex,
    uint32_t leafNormal, uint32_t trunkNormal,
    uint32_t envMap, uint32_t toonRamp)
{
    TreeMaterialHandle handle{};
    auto* device = engine_->GetGraphicsDevice()->GetDevice();

    // 1. 葉用定数バッファ（風のパラメータ含む）の生成
    handle.leafMaterialBuffer = BufferManager::CreateMappedConstantBuffer(
        device, &handle.mappedLeafData
    );
    if (handle.mappedLeafData) {
        *handle.mappedLeafData = leafData;
    }

    // 2. 幹用定数バッファの生成
    handle.trunkMaterialBuffer = BufferManager::CreateMappedConstantBuffer(
        device, &handle.mappedTrunkData
    );
    if (handle.mappedTrunkData) {
        *handle.mappedTrunkData = trunkData;
    }

    // 3. テクスチャハンドルの設定
    handle.leafTextureHandle = leafTex;
    handle.trunkTextureHandle = trunkTex;
    handle.leafNormalMapHandle = leafNormal;
    handle.trunkNormalMapHandle = trunkNormal;
    handle.envMapHandle = envMap;
    handle.toonRampHandle = toonRamp;

    // 寿命管理用リストに保持
    materialBuffers_.push_back(handle.leafMaterialBuffer);
    materialBuffers_.push_back(handle.trunkMaterialBuffer);

    return handle;
}

void TreeSystem::UpdateLeafMaterial(TreeMaterialHandle& handle, const LeafMaterialData& data)
{
    if (handle.mappedLeafData) {
        *handle.mappedLeafData = data; // 高速コピー
    }
}

void TreeSystem::UpdateTrunkMaterial(TreeMaterialHandle& handle, const TrunkMaterialData& data)
{
    if (handle.mappedTrunkData) {
        *handle.mappedTrunkData = data; // 高速コピー
    }
}

void TreeSystem::SetCullingParameters(float maxDrawDistance, float treeHeight, float treeRadius)
{
    // TreeRenderer にパラメータを渡す
    if (engine_ && engine_->GetRendererManager()->GetTreeRenderer())
    {
        engine_->GetRendererManager()->GetTreeRenderer()->SetCullingParameters(maxDrawDistance, treeHeight, treeRadius);
    }
}

}