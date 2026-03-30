#pragma once
#include "Mesh.h"
#include "RenderCommon.h"
#include "RenderEnvironment.h"

namespace FE
{

// 1つのパーティクルの描画に必要な情報をまとめたもの
struct ParticleRequest 
{
    ParticleInstanceData data;
    BlendMode blendMode;
    uint32_t textureIndex;

    // ソート用の比較演算子 (BlendMode->TextureIndexの順で並べる)
    bool operator<(const ParticleRequest& other) const
    {
        if (blendMode != other.blendMode) return blendMode < other.blendMode;
        return textureIndex < other.textureIndex;
    }
};

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
    std::string GetPSOName(BlendMode blendMode);

    static const int32_t kMaxCount = 3000;
    static constexpr int kFrameCount = 3;

    Mesh mesh_;
    Microsoft::WRL::ComPtr<ID3D12Resource> particleInstanceBuffer_[kFrameCount];
    ParticleInstanceData* mappedInstanceData_[kFrameCount] = {};
    int currentFrameIndex_ = 0;

    std::vector<ParticleRequest> requests_;

    uint32_t prevCount_ = 0;
};

}