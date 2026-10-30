#pragma once
#include "RenderCommon.h"
#include "RenderEnvironment.h"

namespace FS
{

class SpriteRenderer
{
public:
    void Initialize(const RenderEnvironment& env, int clientWidth, int clientHeight);

    void BeginFrame();

    // 1 枚分のデータを積む
    void Submit(const SpriteInstance& instance, int layerOrder);

    void Draw(const RenderEnvironment& env);

    uint32_t GetCount() const { return lastDrawCount_; }
    uint32_t GetMaxCount() const { return kMaxInstanceCount; }

private:
    static constexpr uint32_t kMaxInstanceCount = 16384;
    static constexpr uint32_t kFrameBufferCount = 2;  

    // Submit された 1 枚分と、その重なり順
    struct SubmittedSprite
    {
        SpriteInstance instance;
        int layerOrder;
    };

    std::vector<SubmittedSprite> submittedSprites_;

    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, kFrameBufferCount> instanceBuffers_;
    std::array<SpriteInstance*, kFrameBufferCount> mappedInstanceBuffers_{};

    Microsoft::WRL::ComPtr<ID3D12Resource> frameConstantBuffer_;
    SpriteFrameConstants* mappedFrameConstants_ = nullptr;

    uint32_t currentFrameIndex_ = 0;
    uint32_t lastDrawCount_ = 0;
    bool hasWarnedOverflow_ = false;
};

}