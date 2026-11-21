#pragma once
#include "Engine.h"
#include "GameObject.h"
#include "PropertyBinder.h"
#include "Terrain.h"

class DistantMountains : public FS::GameObject
{
public:
    // groupName は設定を保存するグループの名前
    DistantMountains(FS::Engine* engine, const FS::Terrain* playableTerrain, const std::string& groupName);

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugGui() override;

    // 遠景の地面の高さ
    float GetHeight(float worldX, float worldZ) const;
    // 作り直すたびに増える
    uint32_t GetShapeVersion() const { return shapeVersion_; }

private:
    void Generate();
    float CalculateHeight(float worldX, float worldZ, const FS::Vector2& playableMin, const FS::Vector2& playableMax) const;

    FS::Engine* engine_ = nullptr;
    const FS::Terrain* playableTerrain_ = nullptr;
    std::unique_ptr<FS::Terrain> vistaTerrain_;
    std::unique_ptr<FS::PropertyBinder> binder_;

    bool isEnabled_ = true;
    float extent_ = 1000.0f;        
    float cellSize_ = 4.0f;         
    float flatWidth_ = 40.0f;       
    float riseLength_ = 300.0f;     
    float mountainHeight_ = 150.0f;
    float noiseScale_ = 0.004f;     
    float ridgeSharpness_ = 2.0f;   
    int seed_ = 1;

    bool needsRegenerate_ = true;
    // 最後に作ったときの、歩ける地形の形
    uint32_t generatedTerrainVersion_ = 0;
    bool isGenerated_ = false;
    uint32_t shapeVersion_ = 0;
};
