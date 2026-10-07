#pragma once
#include "Mesh.h"
#include "RenderCommon.h"
#include "RenderEnvironment.h"
#include "Frustum.h"

namespace FE
{

class WaterRenderer
{
public:
    void Initialize(const RenderEnvironment& env);
    void Finalize();

    void BeginFrame();
    void SetCameraState(const Matrix4x4& view, const Matrix4x4& viewProjection);

    // シーン背景・深度テクスチャの設定（描画直前に呼び出す）
    void SetSceneTextures(
        D3D12_GPU_DESCRIPTOR_HANDLE sceneColorSRV,
        D3D12_GPU_DESCRIPTOR_HANDLE sceneDepthSRV);

    // 描画登録
    void Submit(
        const WorldTransform& worldTransform,
        const ModelData& modelData,
        D3D12_GPU_VIRTUAL_ADDRESS waterMaterialCBV,
        uint32_t normalMapHandle,
        uint32_t envMapSrvHandle,
        const Vector4& instanceColor = Vector4(1, 1, 1, 1));

    // バッチ化とインスタンシングバッファの生成
    void PrepareBatches();

    // 描画実行
    void Draw(const RenderEnvironment& env, D3D12_GPU_VIRTUAL_ADDRESS interactionCBAddress,
        D3D12_GPU_DESCRIPTOR_HANDLE interactionSrvHandle);

    void Reset()
    {
        waterSubmissions_.clear();
        batches_.clear();
        currentInstanceLocation_ = 0;
    }

private:
    struct ModelBatch
    {
        std::vector<Mesh> meshes;
    };

    struct InstanceBuffer
    {
        Microsoft::WRL::ComPtr<ID3D12Resource> resource;
        Object3DInstanceData* mapped = nullptr;
        uint32_t srvIndex = 0;
    };

    const std::vector<Mesh>& GetOrCreateBatch(const ModelData& modelData);

private:
    static constexpr uint32_t kMaxInstances = 1000;
    GraphicsDevice* device_ = nullptr;

    InstanceBuffer instanceBuffer_;
    std::map<const ModelData*, ModelBatch> meshCache_;
    std::vector<WaterSubmission> waterSubmissions_;
    std::vector<WaterBatch> batches_;

    Matrix4x4 viewMatrix_;
    Matrix4x4 viewProjectionMatrix_;

    D3D12_GPU_DESCRIPTOR_HANDLE sceneColorSRV_{};
    D3D12_GPU_DESCRIPTOR_HANDLE sceneDepthSRV_{};
    uint32_t envMapSrvHandle_ = 0;

    uint32_t currentInstanceLocation_ = 0;
};

}