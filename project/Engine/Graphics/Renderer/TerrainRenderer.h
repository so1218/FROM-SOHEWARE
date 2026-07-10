#pragma once
#include "RenderCommon.h"
#include "RenderEnvironment.h"

namespace FE
{

class TerrainChunk;
class ShadowMap;
    
class TerrainRenderer
{
public:
    void Initialize(const RenderEnvironment& env);
    void Finalize();

    void BeginFrame();

    // カメラ情報の受け取り
    void SetCameraState(const Matrix4x4& view, const Matrix4x4& viewProjection);

    // 地形描画登録
    void Submit(const WorldTransform& worldTransform, const TerrainChunk* chunk,
        const MaterialHandle& material, const Vector4& instanceColor);

    // 描画実行
    void Draw(const RenderEnvironment& env, RenderGroup group, bool isWireFrame, ShadowMap* shadowMap);

    // 影用パスの描画
    void DrawShadow(const RenderEnvironment& env, uint32_t cascadeIndex);

    // 描画前のバッチ準備（Zソートやグループ化）
    void PrepareBatches();

    uint32_t GetCount() const { return prevCount_; }
    uint32_t GetMaxCount() const { return kMaxCount; }

private:
    struct TerrainSubmission
    {
        RenderGroup group;
        const TerrainChunk* chunk = nullptr;
        MaterialHandle materialHandle;

        Matrix4x4 worldMatrix;
        Matrix4x4 wvpMatrix;
        Matrix4x4 worldInverseTranspose;
        Vector4 instancingColor;

        uint32_t instanceIndex = 0;
        BlendMode blendMode = BlendMode::kBlendModeNone;
        CullMode cullMode = CullMode::Back;
        DepthMode depthMode = DepthMode::Write;
        float depth = 0.0f; // Zソート用
    };

    // 地形はインスタンシングしないため、チャンクごとに1つの定数バッファを持つ
    struct PerObjectBuffer
    {
        Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource;
        TransformationMatrix* wvpMapped = nullptr;
    };

private:
    static const int32_t kMaxCount = 1000; // 想定されるチャンクの最大数

    GraphicsDevice* device_ = nullptr;

    std::vector<PerObjectBuffer> perObjectBuffers_;
    std::vector<TerrainSubmission> submissions_;

    uint32_t indexChunk_ = 0;
    uint32_t prevCount_ = 0;

    Matrix4x4 viewMatrix_;
    Matrix4x4 viewProjectionMatrix_;
};

}