#pragma once
#include "Engine.h"
#include "GameObject.h"
#include "PropertyBinder.h"
#include "TreeSystem.h"
#include "Terrain.h"

class TreeField : public FE::GameObject
{
public:
    TreeField(FE::Engine* engine);

    void Initialize() override;
    void Update() override;
    void Draw() override {}
    void DebugDraw() override;

    void SetTerrain(FE::Terrain* terrain) { terrain_ = terrain; }

private:
    void GenerateTrees(); // パラメータに基づいて木をランダム散布生成
    void UpdateMaterials();

private:
    FE::Engine* engine_ = nullptr;
    FE::Terrain* terrain_ = nullptr;

    std::unique_ptr<FE::Model> model_;
    std::unique_ptr<FE::TreeSystem> treeSystem_;
    std::unique_ptr<FE::PropertyBinder> binder_;

    FE::WorldTransform transform_;

    // 生成したマテリアルハンドル（全インスタンスで共有）
    FE::TreeMaterialHandle treeMaterialHandle_;

    // ==========================================
    // 配置・散布用パラメータ（変更で自動再生成）
    // ==========================================
    int treeCount_ = 500;
    float areaWidth_ = 400.0f;
    float areaDepth_ = 400.0f;
    FE::Vector2 areaCenter_{ 0.0f, 0.0f };

    float minScale_ = 0.8f;
    float maxScale_ = 1.4f;
    float colorRandomness_ = 0.15f;

    std::string windMapName_ = "noise_39";
    uint32_t windMapHandle_ = 0;

    // 前フレーム記憶（配置変更検知用）
    FE::Vector3 prevPosition_{ 0.0f, 0.0f, 0.0f };
    int prevTreeCount_ = 0;
    float prevAreaWidth_ = 0.0f;
    float prevAreaDepth_ = 0.0f;
    FE::Vector2 prevAreaCenter_{ 0.0f, 0.0f };
    float prevMinScale_ = 0.0f;
    float prevMaxScale_ = 0.0f;
    float prevColorRandomness_ = 0.0f;

    // ==========================================
    // 葉パラメータ
    // ==========================================
    float gustScale_ = 0.05f;
    float windSpeedMultiplier_ = 1.0f;
    float windStrengthMultiplier_ = 1.0f;
    float gustStrength_ = 1.0f;
    float trunkFlexibility_ = 0.1f;
    float branchFlexibility_ = 0.3f;
    float leafFlutterAmount_ = 0.2f;
    float backfaceFlatten_ = 0.5f;
    float diffuseWrap_ = 0.2f;
    float transmissionDistortion_ = 0.1f;
    float transmissionPower_ = 5.0f;
    float sssStrength_ = 1.0f;
    float alphaCutoff_ = 0.5f;
    float leafShadowDensity_ = 0.8f;     
    FE::Vector3 sssColor_ = { 0.5f, 0.7f, 0.2f };
    float leafShadowNormalBias_ = 0.02f; 
    float treeHeight_ = 10.0f;
    float treeRadius_ = 5.0f;
    float leafShadowBias_ = 0.005f;   
    float baseRoughness_ = 1.0f;
    float baseAO_ = 1.0f;
    float baseThickness_ = 0.1f;
    FE::Vector3 leafColorTint_ = { 1.0f, 1.0f, 1.0f };
    float leafAlbedoMultiplier_ = 1.0f;
    float leafFlutterFrequency_ = 1.0f;

    // ==========================================
    // 幹パラメータ
    // ==========================================
    FE::Vector4 trunkColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };
    FE::Vector4 trunkSpecularColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };
    int trunkEnableLighting_ = 1;
    int trunkLightMode_ = 1;
    int trunkEnableNormalMap_ = 1;
    int trunkAddShadow_ = 1;
    float trunkRoughness_ = 0.8f;
    float trunkMetalness_ = 0.0f;
    float trunkShininess_ = 10.0f;
    float trunkDiffuseReflection_ = 1.0f;
    float trunkShadowDensity_ = 0.8f;    
    float trunkShadowBias_ = 0.005f;    
    float trunkShadowNormalBias_ = 0.02f; 
    float trunkShadowSoftness_ = 1.0f;
    float trunkEnvironmentMapIntensity_ = 1.0f;
    float trunkShadowEnvStrength_ = 0.5f;
    float trunkNormalIntensity_ = 1.0f;
    float trunkAlbedoMultiplier_ = 1.0f;

    float maxDrawDistance_ = 300.0f;

    // 葉
    std::string leafTextureName_ = "white1x1";
    uint32_t    leafTextureHandle_ = 0;
    std::string leafNormalName_ = "white1x1";
    uint32_t    leafNormalHandle_ = 0;

    // 幹 
    std::string trunkTextureName_ = "white1x1";
    uint32_t    trunkTextureHandle_ = 0;
    std::string trunkNormalName_ = "white1x1";
    uint32_t    trunkNormalHandle_ = 0;

    // 共通
    std::string toonRampName_ = "toonRamp_01";
    uint32_t    toonRampHandle_ = 0;
};