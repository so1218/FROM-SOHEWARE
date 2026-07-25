#pragma once
#include "Engine.h"
#include "GameObject.h"
#include "PropertyBinder.h"
#include "GrassSystem.h"
#include "Terrain.h"

class Player;

class GrassField : public FE::GameObject
{
public:
    GrassField(FE::Engine* engine, Player* player);

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugDraw() override;

    void SetTerrain(FE::Terrain* terrain) { terrain_ = terrain; }

private:
    void GenerateGrass();

private:
    FE::Engine* engine_;
    Player* player_;
    FE::Terrain* terrain_ = nullptr;

    std::unique_ptr<FE::GrassSystem> grassSystem_;
    std::unique_ptr<FE::PropertyBinder> binder_;

    FE::WorldTransform transform_;

    // ==========================================
    // GPUに送る 生成用パラメータ（変更されたら再生成が必要）
    // ==========================================
    int maxGrassPerChunk_ = 300000;
    float gridSpacing_ = 0.5f;
    float baseScale_ = 1.0f;

    float minHeight_ = 0.8f;
    float maxHeight_ = 1.5f;
    float minWidth_ = 0.8f;
    float maxWidth_ = 1.5f;

    // 前フレーム比較用（再生成判定のため）
    FE::Vector3 prevPosition_;
    int prevMaxGrassPerChunk_ = 0;
    float prevGridSpacing_ = 0.0f;
    float prevBaseScale_ = 0.0f;
    float prevMinHeight_ = 0.0f;
    float prevMaxHeight_ = 0.0f;
    float prevMinWidth_ = 0.0f;
    float prevMaxWidth_ = 0.0f;

    std::string heightMapName_ = "noise_39";
    uint32_t heightMapHandle_ = 0;

    std::string densityMapName_ = "white1x1";
    uint32_t densityMapHandle_ = 0;

    std::string windMapName_ = "noise_39";
    uint32_t windMapHandle_ = 0;

    // 前回のテクスチャ名を記憶（変更検知による再生成用）
    std::string prevHeightMapName_ = "";
    std::string prevDensityMapName_ = "";

    // 地形フィッティング用パラメータ
    FE::Vector2 terrainCenter_;
    float terrainWidth_ = 500.0f;
    float terrainDepth_ = 500.0f;

    // 変更監視用
    FE::Vector2 prevTerrainCenter_{ 0.0f, 0.0f };
    float prevTerrainWidth_ = 0.0f;
    float prevTerrainDepth_ = 0.0f;
};