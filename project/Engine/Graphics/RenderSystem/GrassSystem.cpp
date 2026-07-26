#include "pch.h"
#include "GrassSystem.h"
#include "Engine.h"
#include "ModelManager.h"

namespace FE
{

GrassSystem::GrassSystem(Engine* engine, const std::string& windMapTextureName)
    : engine_(engine)
{
    engine_->GetRendererManager()->InitializeGrass();
    SetWindMapTexture(windMapTextureName);
}

// GPUでの草一括生成を呼び出す
void GrassSystem::Generate(const GrassGenerationData& genData, const std::string& heightMapName, const std::string& densityMapName)
{
    auto* rendererManager = engine_->GetRendererManager();

    // テクスチャの名前からSRVハンドルを取得
    uint32_t heightMapHandle = TextureManager::GetInstance().Get(heightMapName);
    uint32_t densityMapHandle = TextureManager::GetInstance().Get(densityMapName);

    // RendererManagerにこの設定でGPU上で草を作れと命令
    rendererManager->GenerateGrass(genData, heightMapHandle, densityMapHandle);
}

void GrassSystem::Update()
{
    if (!engine_) return;

    auto* rendererManager = engine_->GetRendererManager();

    // 現在のマテリアルとカリング情報だけをマネージャーに伝達
    rendererManager->SetGrassRenderingParams(windMapTextureHandle_, materialData_, cullingData_);
}

void GrassSystem::SetWindMapTexture(const std::string& textureName)
{
    windMapTextureHandle_ = TextureManager::GetInstance().Get(textureName);
}

}