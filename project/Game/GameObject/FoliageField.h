#pragma once
#include "Engine.h"
#include "GameObject.h"
#include "PropertyBinder.h"
#include "FoliageSystem.h"
#include "Terrain.h"
#include "RenderCommon.h"

class FoliageField : public FE::GameObject
{
public:
    FoliageField(FE::Engine* engine);

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugDraw() override;

private:
    void AddFoliageLayer(const std::string& layerName, const std::string& modelName);
    void SetupBinderForLayer(size_t layerIndex);
    void ReloadResources();
    void GenerateFoliage();

private:
    FE::Engine* engine_;
    std::unique_ptr<FE::FoliageSystem> foliageSystem_;
    std::unique_ptr<FE::PropertyBinder> binder_;

    FE::WorldTransform transform_;

    // ==========================================
    // 全体共通パラメータ
    // ==========================================
    std::string heightMapName_ = "noise_39";
    uint32_t heightMapHandle_ = 0;

    FE::Vector2 terrainCenter_{ 0.0f, 0.0f };
    float terrainWidth_ = 1024.0f;
    float terrainDepth_ = 1024.0f;

    FoliageCullingData cullingData_{};

    std::vector <FE::FoliageLayer> layers_;

    // 再生成判定用キャッシュ
    FE::Vector3 prevPosition_;
    float prevTerrainWidth_ = 0.0f;
    float prevTerrainDepth_ = 0.0f;
    std::string prevHeightMapName_;
};