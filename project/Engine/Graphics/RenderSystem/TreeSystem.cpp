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
    auto* rendererManager = engine_->GetRendererManager();

    rendererManager->SetWindMap(windMapTextureHandle_);

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
    uint32_t leafNormal, uint32_t trunkNormal, uint32_t toonRamp)
{
    TreeMaterialHandle handle{};
    auto* device = engine_->GetGraphicsDevice()->GetDevice();

    handle.leafMaterialBuffer = BufferManager::CreateMappedConstantBuffer(
        device, &handle.mappedLeafData
    );
    if (handle.mappedLeafData) {
        *handle.mappedLeafData = leafData;
    }

    handle.trunkMaterialBuffer = BufferManager::CreateMappedConstantBuffer(
        device, &handle.mappedTrunkData
    );
    if (handle.mappedTrunkData) {
        *handle.mappedTrunkData = trunkData;
    }

    handle.leafTextureHandle = leafTex;
    handle.trunkTextureHandle = trunkTex;
    handle.leafNormalMapHandle = leafNormal;
    handle.trunkNormalMapHandle = trunkNormal;
    handle.toonRampHandle = toonRamp;

    materialBuffers_.push_back(handle.leafMaterialBuffer);
    materialBuffers_.push_back(handle.trunkMaterialBuffer);

    return handle;
}

void TreeSystem::UpdateLeafMaterial(TreeMaterialHandle& handle, const LeafMaterialData& data)
{
    if (handle.mappedLeafData)
    {
        *handle.mappedLeafData = data; 
    }
}

void TreeSystem::UpdateTrunkMaterial(TreeMaterialHandle& handle, const TrunkMaterialData& data)
{
    if (handle.mappedTrunkData)
    {
        *handle.mappedTrunkData = data; 
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