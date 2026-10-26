#pragma once
#include "Engine.h"
#include "GameObject.h"
#include "PropertyBinder.h"
#include "PebbleSystem.h"
#include "Terrain.h"
#include "RenderCommon.h"

class PebbleField : public FS::GameObject
{
public:
    PebbleField(FS::Engine* engine);

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugDraw() override;

private:
    void GeneratePebbles();
    void ReloadResources();

private:
    FS::Engine* engine_;
    std::unique_ptr<FS::PebbleSystem> pebbleSystem_;
    std::unique_ptr<FS::PropertyBinder> binder_;

    FS::WorldTransform transform_;

    // ==========================================
    // 生成用パラメータ (変更されたら再生成)
    // ==========================================
    int maxPebblesPerChunk_ = 100000;
    float gridSpacing_ = 1.0f;
    float minScale_ = 0.5f;
    float maxScale_ = 1.5f;

    // 非等方スケール（X, Y, Zそれぞれのスケール幅）
    FS::Vector3 minAnisoScale_{ 0.8f, 0.3f, 0.8f };
    FS::Vector3 maxAnisoScale_{ 1.2f, 0.8f, 1.2f };

    FS::Vector2 terrainCenter_{ 0.0f, 0.0f };
    float terrainWidth_ = 1024.0f;
    float terrainDepth_ = 1024.0f;

    // 再生成判定用のキャッシュ
    FS::Vector3 prevPosition_;
    int prevMaxPebbles_ = 0;
    float prevGridSpacing_ = 0.0f;
    float prevMinScale_ = 0.0f;
    float prevMaxScale_ = 0.0f;
    FS::Vector3 prevMinAnisoScale_;
    FS::Vector3 prevMaxAnisoScale_;
    float prevTerrainWidth_ = 0.0f;
    float prevTerrainDepth_ = 0.0f;

    // ==========================================
    // リソース関連 (変更されたら再ロード)
    // ==========================================
    std::string pebbleModelName_ = "rock1";

    std::string heightMapName_ = "noise_39";
    std::string densityMapName_ = "white1x1";
    uint32_t heightMapHandle_ = 0;
    uint32_t densityMapHandle_ = 0;

    std::string skyboxName_ = "pureSky";
    std::string albedoMapName_ = "white1x1";
    std::string normalMapName_ = "white1x1";

    uint32_t skyboxHandle_ = 0;
    uint32_t albedoMapHandle_ = 0;
    uint32_t normalMapHandle_ = 0;

    std::string prevHeightMapName_;
    std::string prevDensityMapName_;
};