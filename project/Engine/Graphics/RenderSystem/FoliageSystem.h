#pragma once
#include "WorldTransform.h"
#include "Structures.h"
#include "Mesh.h"

namespace FE
{

class Engine;

class FoliageSystem
{
public:
    FoliageSystem(Engine* engine);
    ~FoliageSystem() = default;

    // 1. アセットロード時に各Foliage(草や花)の種類を追加する
    void AddFoliageType(
        const std::string& albedoTextureName,
        const std::string& normalTextureName,
        const std::string& densityMapName,
        const MeshData& meshData,
        const FoliageMaterialData& defaultMaterial,
        const FoliageGenerationData& defaultGenData);

    // 2. 全種類を追加し終わったら、レンダラー側に初期化を要求する
    void InitializeRenderer();

    // 3. GPU上での全自動生成命令 (マップ切り替え時などに呼ぶ)
    void Generate(
        const std::string& heightMapName,
        UINT terrainWidth, UINT terrainDepth);

    // 4. 毎フレームの更新 (カリングデータなどの送信)
    void Update();

    void UpdateConfigs(const std::vector<FoliageLayer>& layers);

    // デバッグ・パラメーター調整用
    FoliageCullingData* GetCullingData() { return &cullingData_; }
    FoliageMaterialData* GetMaterialData(size_t index);
    FoliageGenerationData* GetGenerationData(size_t index);

private:
    Engine* engine_ = nullptr;
    FoliageCullingData cullingData_{};

    // System側で保持する種類ごとのデータ
    struct FoliageTypeInfo {
        uint32_t albedoSrvHandle = 0;
        uint32_t normalSrvHandle = 0;
        uint32_t densityMapSrvHandle = 0;
        std::unique_ptr<Mesh> mesh; // メッシュの実体をここで保持・管理
        FoliageMaterialData material{};
        FoliageGenerationData genData{};
    };

    std::vector<FoliageTypeInfo> foliageTypes_;
    bool isRendererInitialized_ = false;
};

}