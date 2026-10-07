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

    // 描画リクエストの蓄積。実際の描画コマンド構築は PrepareBatches に遅延させ、
    // フレーム単位でステートの最適化（ソート）を一括で行う
    void Submit(
        const WorldTransform& worldTransform,
        const ModelData& modelData,
        const TreeMaterialHandle& treeMaterial,
        const Vector4& colorVariation,
        float lodFade = 1.0f
    );

    // 蓄積された Submission を評価し、Instancing 可能なバッチに結合
    void PrepareBatches();

    void Draw(const RenderEnvironment& env, ShadowMap* shadowMap, uint32_t windMapSrvIndex);
    void DrawShadow(const RenderEnvironment& env, uint32_t cascadeIndex, uint32_t windMapSrvIndex);

    void Reset()
    {
        submissions_.clear();
        batches_.clear();
        meshCache_.clear(); 
        currentInstanceLocation_ = 0;
    }

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

        // 幹と葉でPSOが異なるため保持
        bool isLeaf = false;

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

    // ExecuteIndirect用
    struct AlignedDrawIndexedArguments
    {
        D3D12_DRAW_INDEXED_ARGUMENTS args;
        uint32_t padding[3];
    };

    const std::vector<Mesh>& GetOrCreateBatch(const ModelData& modelData);

private:
    static constexpr uint32_t kMaxInstances = 2000;
    static constexpr uint32_t kMaxBatches = 256;
    // CPU/GPUの非同期実行時のリソース競合を防ぐためのマルチバッファリング数
    static constexpr uint32_t kFrameCount = 2;
    static constexpr uint32_t kMaxPasses = 5;

    std::vector<TreeSubmission> submissions_;
    std::vector<TreeBatch> batches_;
    std::map<const ModelData*, ModelBatch> meshCache_;

    GraphicsDevice* device_ = nullptr;
    uint32_t currentInstanceLocation_ = 0;

    Microsoft::WRL::ComPtr<ID3D12CommandSignature> commandSignature_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> cullingHeap_;

    // CPUから毎フレーム書き換えるリソースはフレーム単位で独立
    struct FrameResource
    {
        Microsoft::WRL::ComPtr<ID3D12Resource> indirectArgsBuffer;
        Microsoft::WRL::ComPtr<ID3D12Resource> outputInstanceBuffer;
        Microsoft::WRL::ComPtr<ID3D12Resource> cullingDataBuffer;

        Microsoft::WRL::ComPtr<ID3D12Resource> indirectArgsUploadBuffer;
        AlignedDrawIndexedArguments* mappedIndirectArgs = nullptr;

        TreeCullingData* mappedCullingData = nullptr;
        uint32_t outputSrvIndex = 0;

        Microsoft::WRL::ComPtr<ID3D12Resource> inputInstanceBuffer;
        TreeInstanceData* mappedInputInstanceData = nullptr;
    };
    FrameResource frameRes_[kFrameCount];

    uint32_t currentFrameIndex_ = 0;
    uint32_t currentPassIndex_ = 0;

    float currentMaxDrawDistance_ = 1000.0f;
    float currentTreeHeight_ = 10.0f;
    float currentTreeRadius_ = 2.0f;

    Matrix4x4 viewMatrix_{};
    Matrix4x4 viewProjectionMatrix_{};
};

}