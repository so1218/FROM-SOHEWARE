#pragma once
#include "WorldTransform.h"
#include "Structures.h"

namespace FE
{

class Engine;

class GrassSystem
{
public:
    GrassSystem(Engine* engine, const std::string& windMapTextureName);
    ~GrassSystem() = default;

    // 初期化時(またはマップ切り替え時)に1回だけ呼ぶ、GPUへの自動生成命令
    void Generate(const GrassGenerationData& genData, const std::string& heightMapName, const std::string& densityMapName);

    // 毎フレームのパラメータ転送
    void Update();

    // パラメータ設定
    void SetWindMapTexture(const std::string& textureName);
    GrassMaterialData* GetMaterialData() { return &materialData_; }
    GrassCullingData* GetCullingData() { return &cullingData_; }

private:
    Engine* engine_ = nullptr;
    GrassMaterialData materialData_{};
    GrassCullingData cullingData_{};
    uint32_t windMapTextureHandle_ = 0;
};

}