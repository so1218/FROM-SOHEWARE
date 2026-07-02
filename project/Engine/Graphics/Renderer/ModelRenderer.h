#pragma once
#include "Mesh.h"
#include "RenderCommon.h"
#include "ShadowMap.h"
#include "RenderEnvironment.h"

namespace FE
{

class ModelRenderer
{
public:
    void Initialize(const RenderEnvironment& env);
    void Finalize();

    void BeginFrame();

    // カメラ情報の受け取り
    void SetCameraState(const Matrix4x4& view, const Matrix4x4& viewProjection);

    // モデル描画登録
    void Submit(const WorldTransform& worldTransform, const ModelData& modelData,
        const std::vector<MaterialHandle>& materials, BlendMode blendMode, CullMode cullMode,
        DepthMode depthMode, RenderGroup group, const Vector4& instanceColor);

    // アニメーションモデル描画登録
    void SubmitAnimation(
        const WorldTransform& worldTransform,
        const AnimatedModelData& instance,
        const SkinCluster& skinCluster,
        const std::vector<MaterialHandle>& materials,
        BlendMode blendMode,
        RenderGroup group,
        const Vector4& instanceColor);

    // 描画実行
    void Draw(const RenderEnvironment& env, RenderGroup group, bool isWireFrame, ShadowMap* shadowMap);

    // 影用パスの描画
    void DrawShadow(const RenderEnvironment& env, uint32_t cascadeIndex);

    // 描画前のバッチ準備（インスタンシングのためのデータ転送など）
    void PrepareBatches();

    uint32_t GetCount() const { return prevCount_; }
    uint32_t GetMaxCount() const { return kMaxCount; }

private:
    struct ModelBatch
    {
        std::vector<Mesh> meshes;
    };

    struct PerObjectBuffer
    {
        Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource;
        TransformationMatrix* wvpMapped = nullptr;
        Microsoft::WRL::ComPtr<ID3D12Resource> outlineResource;
    };

    struct InstanceBuffer
    {
        Microsoft::WRL::ComPtr<ID3D12Resource> resource;
        Object3DInstanceData* mapped = nullptr;
        uint32_t srvIndex = 0; 
    };

    // キャッシュ取得
    const std::vector<Mesh>& GetOrCreateBatch(const ModelData& modelData);

    // 実際の描画コマンド
    void DrawCore(const RenderEnvironment& env, const ModelSubmission& sub, bool isWireFrame, ShadowMap* shadowMap,
        uint32_t instanceCount, uint32_t startInstanceLocation);

private:
    static const int32_t kMaxCount = 10000; // Submitの最大数
    static constexpr uint32_t kMaxInstances = 10000; // インスタンシングの最大数
    InstanceBuffer instanceBuffer_;

    // メッシュ生成用にデバイスだけは保持しておく
    GraphicsDevice* device_ = nullptr;

    std::map<const ModelData*, ModelBatch> meshCache_;
    std::vector<PerObjectBuffer> perObjectBuffers_;
    std::vector<ModelSubmission> modelSubmissions_;

    uint32_t indexModel_ = 0;
    uint32_t prevCount_ = 0;

    Matrix4x4 viewMatrix_;
    Matrix4x4 viewProjectionMatrix_;

    uint32_t currentInstanceLocation_ = 0;

    std::vector<RenderBatch> batches_;

    int totalDrawCalls = 0;
};

}