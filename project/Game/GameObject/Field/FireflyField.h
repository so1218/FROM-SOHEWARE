#pragma once
#include "Engine.h"
#include "GameObject.h"
#include "PropertyBinder.h"
#include "Terrain.h"

class FireflyField : public FS::GameObject
{
public:
    // groupName は設定を保存するグループの名前
    FireflyField(FS::Engine* engine, const FS::Terrain* terrain, const FS::WorldTransform* playerTransform, const std::string& groupName);

    void Initialize() override;
    void Update() override;
    void Draw() override {}
    void DebugGui() override;

private:
    // 蛍の集まる場所
    struct Cluster
    {
        FS::Vector3 center;   
        float radius = 0.0f;
        float blinkPhase = 0.0f;
    };

    // 夜ほど 1、昼と雨では 0
    float CalculateActivity() const;
    void CollectClusters(const FS::Vector3& playerPosition);
    void SubmitFireflies(const FS::Vector3& playerPosition, float activity);
    void SubmitClusterLights(const FS::Vector3& playerPosition, float activity);
    // プレイヤーを中心に、近いマスから順に並べたマスのずれ
    void RebuildCellOffsets();

    FS::Engine* engine_ = nullptr;
    const FS::Terrain* terrain_ = nullptr;
    const FS::WorldTransform* playerTransform_ = nullptr;
    std::unique_ptr<FS::PropertyBinder> binder_;

    uint32_t textureHandle_ = 0;
    float elapsedTime_ = 0.0f;
    std::vector<Cluster> clusters_;
    std::vector<std::pair<int, int>> cellOffsets_;
    float builtActiveRadius_ = -1.0f;
    float builtCellSize_ = -1.0f;
    int submittedCount_ = 0;
    int submittedLightCount_ = 0;

    // 居場所と数
    float cellSize_ = 4.0f;             
    float baseDensity_ = 0.004f;        
    float clusterDensity_ = 0.12f;      
    float clusterCellSize_ = 40.0f;     
    float clusterProbability_ = 0.35f;  
    float clusterRadiusMin_ = 5.0f;
    float clusterRadiusMax_ = 12.0f;
    float heightMin_ = 0.3f;            
    float heightMax_ = 2.0f;

    // 近くだけ動かす範囲
    float activeRadius_ = 30.0f;
    float fadeStartRatio_ = 0.6f;       
    int maxCount_ = 200;

    // 漂う動き
    float driftDistance_ = 1.2f;        
    float driftHeight_ = 0.4f;          
    float driftSpeed_ = 0.12f;          

    // 明滅
    float blinkPeriodMin_ = 2.0f;
    float blinkPeriodMax_ = 5.0f;
    float glowDuration_ = 0.8f;
    float dimBrightness_ = 0.05f;       
    float clusterSync_ = 0.6f;          

    // 見た目
    FS::Vector4 color_ = { 0.75f, 1.0f, 0.3f, 1.0f };
    float size_ = 0.08f;
    float intensity_ = 8.0f;

    // 出る時間と天気
    float nightStartSunHeight_ = 0.05f;  
    float nightFullSunHeight_ = -0.1f;   
    float rainCutoff_ = 0.2f;            

    // 集まりのライト
    int maxLightCount_ = 2;
    float lightDistance_ = 25.0f;        
    FS::Vector4 lightColor_ = { 0.7f, 1.0f, 0.35f, 1.0f };
    float lightIntensity_ = 1.5f;
    float lightRadius_ = 8.0f;
    float lightHeight_ = 1.0f;
    float lightVolumetricIntensity_ = 1.0f;
};
