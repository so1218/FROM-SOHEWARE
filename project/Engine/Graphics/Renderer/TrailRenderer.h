#pragma once
#include "Mesh.h"
#include "RenderCommon.h"
#include "RenderEnvironment.h"
#include "ParticleDefinition.h"

namespace FE
{

class TrailRenderer
{
public:
    void Initialize(const RenderEnvironment& env);
    void BeginFrame();

    void Submit(const std::vector<TrailPoint>& points, const TrailModule& config, const Vector3& cameraPosition,         // 共通データ（カメラ位置）
        float instanceSeed);

    void Draw(const RenderEnvironment& env, const Matrix4x4& viewProjection);

    uint32_t GetCount() const { return prevTrailCount_; }
    uint32_t GetMaxCount() const { return kMaxTrailCount; }

private:
    static const int32_t kMaxTrailCount = 800;
    static const int32_t kMaxTrailVertices = 512;

    struct TrailBatch
    {
        uint32_t startVertexIndex;
        uint32_t vertexCount;
        uint32_t textureHandle;
        uint32_t dissolveHandle;
        TrailMaterialData materialData;
    };

    struct TrailBatchResource
    {
        Mesh mesh;
        std::vector<VertexDataTrail> verticesCPU;
        Microsoft::WRL::ComPtr<ID3D12Resource> materialResource;
        TrailMaterialData* mappedMaterial = nullptr;
        Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource;
        TransformationMatrix* mappedWvp = nullptr;
    } trailBatch_;

    std::vector<TrailBatch> trailBatches_;

    uint32_t indexTrail_ = 0;
    uint32_t prevTrailCount_ = 0;
};

}