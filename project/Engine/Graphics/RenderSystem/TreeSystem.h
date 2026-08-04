#pragma once
#include "WorldTransform.h"
#include "Structures.h"

namespace FE
{

class Engine;

// 木のインスタンス情報
struct TreeInstance
{
    WorldTransform transform;
    const ModelData* modelData = nullptr;
    TreeMaterialHandle treeMaterial;
    Vector4 colorVariation{ 1.0f, 1.0f, 1.0f, 1.0f };
    float lodFade = 1.0f;
};

class TreeSystem
{
public:
    TreeSystem(Engine* engine, const std::string& windMapTextureName = "noise_39");
    ~TreeSystem() = default;

    // 風テクスチャの設定
    void SetWindMapTexture(const std::string& textureName);

    // インスタンスのクリアと追加
    void Clear();
    void AddInstance(const TreeInstance& instance);
    void AddInstance(
        const WorldTransform& transform,
        const ModelData& modelData,
        const TreeMaterialHandle& treeMaterial,
        const Vector4& colorVariation = { 1.0f, 1.0f, 1.0f, 1.0f },
        float lodFade = 1.0f
    );

    // 毎フレーム呼び出し：風設定を伝えつつ、登録されている木を全件 Submit する
    void Update();

    // ★追加: 幹・葉のマテリアルを生成するヘルパー関数
    TreeMaterialHandle CreateTreeMaterial(
        const LeafMaterialData& leafData,
        const TrunkMaterialData& trunkData,
        uint32_t leafTex, uint32_t trunkTex,
        uint32_t leafNormal = 0, uint32_t trunkNormal = 0,
        uint32_t envMap = 0, uint32_t toonRamp = 0
    );

    // ★追加: マテリアルのパラメータを後から動的に更新する関数（時間帯の変化などに対応）
    void UpdateLeafMaterial(TreeMaterialHandle& handle, const LeafMaterialData& data);
    void UpdateTrunkMaterial(TreeMaterialHandle& handle, const TrunkMaterialData& data);
    void SetCullingParameters(float maxDrawDistance, float treeHeight, float treeRadius);

    uint32_t GetInstanceCount() const { return static_cast<uint32_t>(instances_.size()); }

private:
    Engine* engine_ = nullptr;
    std::vector<TreeInstance> instances_;
    uint32_t windMapTextureHandle_ = 0;

    // 定数バッファリソースの寿命管理用リスト
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> materialBuffers_;
};

}