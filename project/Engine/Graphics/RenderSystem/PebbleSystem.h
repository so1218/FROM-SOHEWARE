#pragma once
#include "WorldTransform.h"
#include "Structures.h"
#include "Mesh.h"

namespace FE
{

class Engine;

class PebbleSystem
{
public:
    PebbleSystem(Engine* engine);
    ~PebbleSystem() = default;

    // 初期化時(またはアセットロード時)に呼ぶリソース設定
    void SetResources(
        const std::string& skyboxTextureName,
        const std::string& albedoArrayName,
        const std::string& normalArrayName,
        const MeshData& pebbleMeshData);

    // GPU上での全自動生成命令 (マップ切り替え時や初期化時に1回呼ぶ)
    void Generate(
        const PebbleGenerationData& genData,
        const std::string& heightMapName,
        const std::string& densityMapName);

    // 毎フレームのパラメータ更新・転送
    void Update();

    // デバッグ・パラメーター調整用アクセサ
    PebbleMaterialData* GetMaterialData() { return &materialData_; }
    PebbleCullingData* GetCullingData() { return &cullingData_; }

private:
    Engine* engine_ = nullptr;

    PebbleMaterialData materialData_{};
    PebbleCullingData cullingData_{};

    // リソース情報（ハンドルとメッシュのキャッシュ）
    uint32_t skyboxSrvHandle_ = 0;
    uint32_t albedoArraySrvHandle_ = 0;
    uint32_t normalArraySrvHandle_ = 0;
    Mesh pebbleMesh_{};

};

}