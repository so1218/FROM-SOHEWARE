#pragma once
#include "Engine.h"
#include "GameObject.h"
#include "PropertyBinder.h"
#include "GrassSystem.h"
#include "Terrain.h"

class GrassField : public FS::GameObject
{
public:
    GrassField(FS::Engine* engine, FS::Terrain* terrain);

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugGui() override;

private:
    void GenerateGrass();

private:
    FS::Engine* engine_;
    FS::Terrain* terrain_ = nullptr;

    std::unique_ptr<FS::GrassSystem> grassSystem_;
    std::unique_ptr<FS::PropertyBinder> binder_;

    // 生成に使う設定
    int maxGrassCount_ = 300000;
    float gridSpacing_ = 0.5f;
    float baseScale_ = 1.0f;

    float minHeight_ = 0.8f;
    float maxHeight_ = 1.5f;
    float minWidth_ = 0.8f;
    float maxWidth_ = 1.5f;

    std::string densityMapName_ = "white1x1";
    uint32_t densityMapHandle_ = 0;

    std::string windMapName_ = "noise_39";
    uint32_t windMapHandle_ = 0;

    bool needsRegenerate_ = true;
    // 最後に生成したときの地形の形
    uint32_t generatedTerrainVersion_ = 0;
};