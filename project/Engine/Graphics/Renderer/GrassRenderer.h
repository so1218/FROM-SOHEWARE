#pragma once
#include "Mesh.h"
#include "RenderCommon.h"
#include "ShadowMap.h"
#include "RenderEnvironment.h"

class GrassRenderer 
{
public:
    void Initialize(const RenderEnvironment& env, const ModelData& grassModel);
    void BeginFrame();

    // 描画リクエストの受付
    void Submit(const Matrix4x4& world, const Vector4& color);

    // 描画実行
    void Draw(const RenderEnvironment& env, uint32_t textureHandle, ShadowMap* shadowMap, const MaterialData& materialData);

private:
    static const int32_t kMaxInstances = 10000; 
    static constexpr int kFrameCount = 3;

    Mesh mesh_; // 草の形状（1つ分のメッシュ）

    // インスタンスバッファ
    Microsoft::WRL::ComPtr<ID3D12Resource> instanceBuffer_[kFrameCount];
    GrassInstanceData* mappedInstanceData_[kFrameCount] = {};

    // マテリアルバッファ
    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_[kFrameCount];
    MaterialData* mappedMaterial_[kFrameCount] = {};

    int currentFrameIndex_ = 0;
    std::vector<GrassInstanceData> instanceQueue_;
};