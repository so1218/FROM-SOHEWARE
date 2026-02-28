#pragma once
#include "Mesh.h"
#include "WorldTransform.h"
#include "RenderCommon.h"
#include "MaterialManager.h"
#include "ShadowMap.h"
#include "AnimationData.h"
#include "RenderEnvironment.h"
#include <vector>
#include <map>
#include <string>
#include <functional>
#include <wrl/client.h>
#include <d3d12.h>

class ModelRenderer
{
public:
    void Initialize(const RenderEnvironment& env);
    void Finalize();

    void BeginFrame();

    // カメラ情報の受け取り
    void SetCameraState(const Matrix4x4& view, const Matrix4x4& viewProjection);

    // モデル描画登録
    void SubmitModel(const WorldTransform& worldTransform, const ModelData& modelData,
        const std::vector<MaterialHandle>& materials, BlendMode blendMode, CullMode cullMode,
        DepthMode depthMode, RenderGroup group, const Vector4& instanceColor);

    // アニメーションモデル描画登録
    void SubmitAnimationModel(
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
    void DrawShadow(const RenderEnvironment& env);

    uint32_t GetModelCount() const { return prevModelCount_; }
    uint32_t GetMaxModelCount() const { return kMaxModelCount; }

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

    // キャッシュ取得
    const std::vector<Mesh>& GetOrCreateModelBatch(const ModelData& modelData);

    // 実際の描画コマンド
    void DrawModelCore(const RenderEnvironment& env, const ModelSubmission& sub, bool isWireFrame, ShadowMap* shadowMap);

private:
    static const int32_t kMaxModelCount = 500;

    // メッシュ生成用にデバイスだけは保持しておく
    GraphicsDevice* device_ = nullptr;

    std::map<const ModelData*, ModelBatch> meshCache_;
    std::vector<PerObjectBuffer> perObjectBuffers_;
    std::vector<ModelSubmission> modelSubmissions_;

    uint32_t indexModel_ = 0;
    uint32_t prevModelCount_ = 0;

    Matrix4x4 viewMatrix_;
    Matrix4x4 viewProjectionMatrix_;
};