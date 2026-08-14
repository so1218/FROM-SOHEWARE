#include "pch.h"
#include "PebbleSystem.h"
#include "Engine.h"
#include "TextureManager.h"

namespace FE
{

PebbleSystem::PebbleSystem(Engine* engine)
{
    engine_ = engine;
}

void PebbleSystem::SetResources(
    const std::string& skyboxTextureName,
    const std::string& albedoArrayName,
    const std::string& normalArrayName,
    const MeshData& pebbleMeshData)
{
    skyboxSrvHandle_ = TextureManager::GetInstance().Get(skyboxTextureName);
    albedoArraySrvHandle_ = TextureManager::GetInstance().Get(albedoArrayName);
    normalArraySrvHandle_ = TextureManager::GetInstance().Get(normalArrayName);
    auto* device = engine_->GetGraphicsDevice()->GetDevice();

    pebbleMesh_.Initialize(device, pebbleMeshData.vertices, pebbleMeshData.indices);
    pebbleMesh_.SetVertexCount(static_cast<uint32_t>(pebbleMeshData.vertices.size()));
    pebbleMesh_.SetIndexCount(static_cast<uint32_t>(pebbleMeshData.indices.size()));
    uint32_t indexCount = pebbleMesh_.GetIndexCount();
}

void PebbleSystem::Update()
{
    if (!engine_) return;

    auto* rendererManager = engine_->GetRendererManager();

    // 毎フレーム最新のパラメータを RendererManager に送信
    rendererManager->SetPebbleRenderingParams(
        skyboxSrvHandle_,
        albedoArraySrvHandle_,
        normalArraySrvHandle_,
        pebbleMesh_,
        materialData_,
        cullingData_
    );
}

void PebbleSystem::Generate(
    const PebbleGenerationData& genData,
    const std::string& heightMapName,
    const std::string& densityMapName)
{
    if (!engine_) return;

    auto* rendererManager = engine_->GetRendererManager();

    uint32_t heightMapHandle = TextureManager::GetInstance().Get(heightMapName);
    uint32_t densityMapHandle = TextureManager::GetInstance().Get(densityMapName);

    rendererManager->GeneratePebbles(genData, heightMapHandle, densityMapHandle);
}

}