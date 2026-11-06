#pragma once
#include "Engine.h"
#include "GameObject.h"
#include "PropertyBinder.h"
#include "FoliageSystem.h"
#include "Terrain.h"
#include "RenderCommon.h"

class FoliageField : public FS::GameObject
{
public:
    FoliageField(FS::Engine* engine);

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugGui() override;

    void SetTerrain(FS::Terrain* terrain) { terrain_ = terrain; }

private:
    void AddFoliageLayer(const std::string& layerName, const std::string& modelName);
    void SetupBinderForLayer(size_t layerIndex);
    void GenerateFoliage();

private:
    FS::Engine* engine_;
    FS::Terrain* terrain_ = nullptr;
    std::unique_ptr<FS::FoliageSystem> foliageSystem_;
    std::unique_ptr<FS::PropertyBinder> binder_;

    FoliageCullingData cullingData_{};

    // 生える範囲は地形全体なので、各層の密度マップも地形全体に貼られる
    std::vector<FS::FoliageLayer> layers_;

    bool needsRegenerate_ = true;
    // 最後に生成したときの地形の形。変わっていたら作り直す
    uint32_t generatedTerrainVersion_ = 0;
};