#pragma once
#include "Mesh.h"
#include "RenderCommon.h"
#include "RenderEnvironment.h"

class ParticleRenderer
{
public:
    void Initialize(const RenderEnvironment& env);
    void BeginFrame();

    void Submit(const WorldTransform& worldTransform, uint32_t color, uint32_t textureIndex, float rotationZ,
        BlendMode blendMode, bool isBillboard, float intensity);

    void Draw(const RenderEnvironment& env);

    uint32_t GetCount() const { return prevCount_; }
    uint32_t GetMaxCount() const { return kMaxCount; }

private:
    static const int32_t kMaxCount = 3000;
    static constexpr int kFrameCount = 3;

    Mesh mesh_;
    Microsoft::WRL::ComPtr<ID3D12Resource> particleInstanceBuffer_[kFrameCount];
    ParticleInstanceData* mappedInstanceData_[kFrameCount] = {};
    int currentFrameIndex_ = 0;

    // ブレンドモードとテクスチャIDごとのバッチ
    std::map<BlendMode, std::map<uint32_t, std::vector<ParticleInstanceData>>> batches_;

    uint32_t index_ = 0;
    uint32_t prevCount_ = 0;
};