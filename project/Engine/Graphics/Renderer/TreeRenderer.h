#pragma once
#include "Mesh.h"
#include "RenderCommon.h"
#include "ShadowMap.h"
#include "RenderEnvironment.h"

namespace FE
{

class TreeRenderer
{
public:
    void Initialize(const RenderEnvironment& env);
    void BeginFrame();

    // 木の描画登録 (1回の呼び出しで幹と葉の両方を内部で振り分ける)
    void Submit(
        const WorldTransform& worldTransform,
        const ModelData& modelData,
        const TreeMaterialHandle& treeMaterial,
        const Vector4& colorVariation,
        float lodFade = 1.0f
    );

    void PrepareBatches();

    // メイン描画（幹と葉をそれぞれ最適なPSOで描画）
    void Draw(const RenderEnvironment& env, ShadowMap* shadowMap, uint32_t windMapSrvIndex);

    // 影用パス描画（幹と葉でそれぞれ影を描画）
    void DrawShadow(const RenderEnvironment& env, uint32_t cascadeIndex, uint32_t windMapSrvIndex);

    void SetCullingParameters(float maxDrawDistance, float treeHeight, float treeRadius);
    uint32_t GetCount() const { return static_cast<uint32_t>(submissions_.size()); }
    uint32_t GetMaxCount() const { return kMaxInstances; }

    void SetCameraState(const Matrix4x4& view, const Matrix4x4& viewProjection)
    {
        viewMatrix_ = view;
        viewProjectionMatrix_ = viewProjection;
    }
private:

    struct InstanceBuffer
    {
        Microsoft::WRL::ComPtr<ID3D12Resource> resource;
        TreeInstanceData* mapped = nullptr;
        uint32_t srvIndex = 0;
    };

    struct TreeSubmission
    {
        const ModelData* modelData = nullptr;
        uint32_t meshIndex = 0;
        TreeMaterialHandle treeMaterial;
        bool isLeaf = false; // 葉っぱか幹かの判定フラグ

        uint32_t envMapSrvHandle = 0;
        uint32_t toonRampHandle = 0;

        Matrix4x4 worldMatrix;
        Vector4 colorVariation;
        float lodFade = 1.0f;
    };

    struct TreeBatch
    {
        const ModelData* modelData = nullptr;
        uint32_t meshIndex = 0;
        TreeMaterialHandle treeMaterial;
        bool isLeaf = false;

        uint32_t envMapSrvHandle = 0;
        uint32_t toonRampHandle = 0;

        uint32_t instanceCount = 0;
        uint32_t startInstanceLocation = 0;
    };

    struct ModelBatch
    {
        std::vector<Mesh> meshes;
    };

    struct AlignedDrawIndexedArguments
    {
        D3D12_DRAW_INDEXED_ARGUMENTS args;
        UINT padding[3];
    };

    // キャッシュ取得
    const std::vector<Mesh>& GetOrCreateBatch(const ModelData& modelData);

private:
    static constexpr uint32_t kMaxInstances =  2000; // 最大インスタンス数
    static constexpr uint32_t kMaxBatches = 256;      // 想定される最大バッチ数
    static constexpr uint32_t kFrameCount = 2;
    static constexpr uint32_t kMaxPasses = 5;

    std::vector<TreeSubmission> submissions_;
    std::vector<TreeBatch> batches_;
    std::map<const ModelData*, ModelBatch> meshCache_;

    GraphicsDevice* device_ = nullptr;
    uint32_t currentInstanceLocation_ = 0;

    Microsoft::WRL::ComPtr<ID3D12CommandSignature> commandSignature_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> cullingHeap_;

    // ★削除: クラス直下の indirectArgsUploadBuffer_ と mappedIndirectArgs_ は FrameResource に移動します

    // フレームごとのリソース
    struct FrameResource
    {
        Microsoft::WRL::ComPtr<ID3D12Resource> indirectArgsBuffer;
        Microsoft::WRL::ComPtr<ID3D12Resource> outputInstanceBuffer;
        Microsoft::WRL::ComPtr<ID3D12Resource> cullingDataBuffer;

        // ★追加: CPU側のUploadバッファもフレーム/パスごとに分離
        Microsoft::WRL::ComPtr<ID3D12Resource> indirectArgsUploadBuffer;
        AlignedDrawIndexedArguments* mappedIndirectArgs = nullptr;

        TreeCullingData* mappedCullingData = nullptr;
        uint32_t outputSrvIndex = 0;

        Microsoft::WRL::ComPtr<ID3D12Resource> inputInstanceBuffer;
        TreeInstanceData* mappedInputInstanceData = nullptr;
    };
    FrameResource frameRes_[kFrameCount];

    uint32_t currentFrameIndex_ = 0;
    // ★追加: 現在のフレーム内で何回目の描画パスかをカウントする
    uint32_t currentPassIndex_ = 0;

    float currentMaxDrawDistance_ = 1000.0f;
    float currentTreeHeight_ = 10.0f;
    float currentTreeRadius_ = 2.0f;

    Matrix4x4 viewMatrix_{};
    Matrix4x4 viewProjectionMatrix_{};
};

}