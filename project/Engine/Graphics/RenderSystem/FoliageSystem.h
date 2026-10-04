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
    explicit FoliageSystem(Engine* engine);
    ~FoliageSystem() = default;

    // 描画パイプライン構築前に草木のアセットと生成パラメータを登録
    // VRAM上のメッシュリソースのライフサイクルは本システムで一元管理する
    void AddFoliageType(
        const std::string& albedoTextureName,
        const std::string& densityMapName,
        const MeshData& meshData,
        const FoliageMaterialData& defaultMaterial,
        const FoliageGenerationData& defaultGenData);

    // 登録済みの型情報からGPU側のストラクチャードバッファ群を確保し、バインド状態を確定
    // 起動時およびロードシーケンスでのみコールされる想定
    void InitializeRenderer();

    // 密度マップとハイトマップを参照し、GPU Computeを用いて地形上にインスタンスを静的生成
    // シームレスロード時の裏読みなど、非同期実行に対応できる設計を想定
    void Generate(
        const std::string& heightMapName,
        uint32_t terrainWidth, uint32_t terrainDepth);

    // 視錐台やLOD算出用のカリング定数をGPUへ送出
    void Update();

    // エディタからパラメータのみ即時反映
    void UpdateConfigs(const std::vector<FoliageLayer>& layers);

    [[nodiscard]] FoliageCullingData* GetCullingData() { return &cullingData_; }
    [[nodiscard]] FoliageMaterialData* GetMaterialData(size_t index);
    [[nodiscard]] FoliageGenerationData* GetGenerationData(size_t index);

private:
    Engine* engine_ = nullptr;
    FoliageCullingData cullingData_{};

    // レンダラーへ渡す状態と、システム側で保持するリソースのバインディング情報
    struct FoliageTypeInfo {
        uint32_t albedoSrvHandle = 0;
        uint32_t densityMapSrvHandle = 0;
        std::unique_ptr<Mesh> mesh;
        FoliageMaterialData material{};
        FoliageGenerationData genData{};
    };

    std::vector<FoliageTypeInfo> foliageTypes_;
    bool isRendererInitialized_ = false;
};

}