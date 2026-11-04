#pragma once
#include "RenderCommon.h"
#include "Structures.h"
#include "RenderEnvironment.h"

namespace FS
{

// 雷を、カメラを向いた細い板の連なりとして描く。
// 形と明るさは LightningSystem が毎フレーム Submit する
class LightningRenderer
{
public:
    void Initialize(const RenderEnvironment& env);
    void BeginFrame();

    // brightness は消えていく分と明滅を掛けたもの
    void Submit(const std::vector<LightningPath>& paths, float brightness);
    void Draw(const RenderEnvironment& env, const Vector3& cameraPosition);

    void SetConfig(const LightningConfig& config) { config_ = config; }

private:
    struct Submission
    {
        std::vector<LightningPath> paths;
        float brightness = 1.0f;
    };

    static constexpr int kFrameCount = 3;
    static constexpr uint32_t kMaxVertices = 10000;
    static constexpr uint32_t kMaxIndices = 15000;

    int currentFrameIndex_ = 0;
    std::vector<Submission> submissions_;
    LightningConfig config_;

    Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_[kFrameCount];
    LightningVertexData* mappedVertices_[kFrameCount] = {};

    Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer_[kFrameCount];
    uint32_t* mappedIndices_[kFrameCount] = {};

    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_[kFrameCount];
    LightningMaterial* mappedMaterial_[kFrameCount] = {};
};

}
