#pragma once
#include "Engine.h"
#include "GameObject.h"
#include "PropertyBinder.h"
#include "TreeSystem.h"
#include "Terrain.h"

struct TreeCollider
{
    FS::Vector3 position;
    float radius;
};

struct TreeRaycastHit
{
    FS::Vector3 point;   // 着弾座標
    FS::Vector3 normal;  // 着弾面の法線（外向きベクトル）
    float distance;      // レイの始点からの距離
};

class TreeField : public FS::GameObject
{
public:
    TreeField(FS::Engine* engine, FS::Terrain* terrain);

    void Initialize() override;
    void Update() override;
    void Draw() override {}
    void DebugGui() override;

    bool ResolveCollision(FS::Vector3& playerPos, float playerRadius) const;
    bool Raycast(const FS::Vector3& rayStart, const FS::Vector3& rayDir, float maxDistance, TreeRaycastHit* outHit) const;

private:
    void GenerateTrees();
    // 葉と幹のマテリアル、カリングの値を GPU 側へ送る
    void ApplyMaterials();
    void ApplyTextures();

    FS::Engine* engine_ = nullptr;
    FS::Terrain* terrain_ = nullptr;

    std::unique_ptr<FS::Model> model_;
    std::unique_ptr<FS::TreeSystem> treeSystem_;
    std::unique_ptr<FS::PropertyBinder> binder_;

    std::vector<TreeCollider> colliders_;
    FS::TreeMaterialHandle treeMaterialHandle_;   // 全インスタンスで共有

    // 配置 (変えると木を作り直す)
    int treeCount_ = 500;
    float areaWidth_ = 400.0f;
    float areaDepth_ = 400.0f;
    FS::Vector2 areaCenter_ = { 0.0f, 0.0f };
    float minScale_ = 0.8f;
    float maxScale_ = 1.4f;
    float trunkColliderRadius_ = 0.5f;   // 大きさ 1 の木の幹の当たり判定の半径

    // GPU に送る値をそのまま持ち、ImGui からも直接変える
    LeafMaterialData leafMaterial_{};
    TrunkMaterialData trunkMaterial_{};

    // カリング
    float maxDrawDistance_ = 300.0f;
    float treeRadius_ = 5.0f;

    // テクスチャ
    std::string windMapName_ = "noise_39";
    uint32_t windMapHandle_ = 0;
    std::string leafTextureName_ = "white1x1";
    uint32_t leafTextureHandle_ = 0;
    std::string trunkTextureName_ = "white1x1";
    uint32_t trunkTextureHandle_ = 0;
    std::string trunkNormalName_ = "white1x1";
    uint32_t trunkNormalHandle_ = 0;
};
