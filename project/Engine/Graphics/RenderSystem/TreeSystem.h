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
    explicit TreeSystem(Engine* engine, const std::string& windMapTextureName = "noise_39");
    ~TreeSystem() = default;

    // 頂点シェーダで群葉の揺れ（Windアニメーション）を計算するためのグローバルノイズマップ
    void SetWindMapTexture(const std::string& textureName);

    void Clear();
    void AddInstance(const TreeInstance& instance);
    void AddInstance(
        const WorldTransform& transform,
        const ModelData& modelData,
        const TreeMaterialHandle& treeMaterial,
        const Vector4& colorVariation = { 1.0f, 1.0f, 1.0f, 1.0f },
        float lodFade = 1.0f
    );

    // 登録されたインスタンス群をレンダラキューへ積む
    void Update();

    // 葉(と幹で分離された専用マテリアルバッファを生成し、ライフサイクルを管理
    [[nodiscard]] TreeMaterialHandle CreateTreeMaterial(
        const LeafMaterialData& leafData,
        const TrunkMaterialData& trunkData,
        uint32_t leafTex, uint32_t trunkTex,
        uint32_t leafNormal = 0, uint32_t trunkNormal = 0, uint32_t toonRamp = 0
    );

    // 時間帯変化や天候に応じた動的パラメータ更新用
    void UpdateLeafMaterial(TreeMaterialHandle& handle, const LeafMaterialData& data);
    void UpdateTrunkMaterial(TreeMaterialHandle& handle, const TrunkMaterialData& data);

    void SetCullingParameters(float maxDrawDistance, float treeHeight, float treeRadius);

    [[nodiscard]] uint32_t GetInstanceCount() const { return static_cast<uint32_t>(instances_.size()); }

private:
    Engine* engine_ = nullptr;
    std::vector<TreeInstance> instances_;
    uint32_t windMapTextureHandle_ = 0;

    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> materialBuffers_;
};

}