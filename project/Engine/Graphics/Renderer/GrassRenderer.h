#pragma once
#include "Mesh.h"
#include "RenderCommon.h"
#include "ShadowMap.h"
#include "RenderEnvironment.h"

namespace FE
{

class GrassRenderer 
{
public:
    void Initialize(const RenderEnvironment& env);
    void BeginFrame();

    // 描画リクエストの受付
    void Submit(const Vector3& position, float height, float rotationY, float width, uint32_t packedColor);

    // 描画実行
    void Draw(const RenderEnvironment& env, uint32_t windMapTextureHandle, ShadowMap* shadowMap, const GrassMaterialData& materialData);

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

}