#pragma once
#include "Mesh.h"
#include "RenderCommon.h"
#include "RenderEnvironment.h"

class SpriteRenderer
{
public:
    void Initialize(const RenderEnvironment& env, int clientWidth, int clientHeight);

    void BeginFrame();

    void Submit(
        const Vector2& position, const Vector2& size, float rotation, uint32_t color,
        const Vector2& anchorPoint, const WorldTransform& uvTransform,
        uint32_t textureHandle, uint32_t dissolveTextureHandle, int layerOrder,
        const MaterialHandle& materialHandle);

    // 描画実行関数
    void Draw(const RenderEnvironment& env);

    uint32_t GetCount() const { return prevCount_; }
    uint32_t GetMaxCount() const { return kMaxCount; }

private:
    static const int32_t kMaxCount = 101;

    struct SpriteData 
    {
        Mesh mesh;
        Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource;
        TransformationMatrix* mappedData = nullptr;
    };

    std::vector<SpriteData> sprites_;
    std::vector<SpriteSubmission> submissions_;

    uint32_t index_ = 0;
    uint32_t prevCount_ = 0;

    int clientWidth_ = 0;
    int clientHeight_ = 0;
};