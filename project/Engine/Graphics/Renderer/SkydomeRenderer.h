#pragma once
#include "Mesh.h"
#include "RenderCommon.h"
#include "RenderEnvironment.h"

namespace FE
{

class SkydomeRenderer
{
public:
    void Initialize(const RenderEnvironment& env);
    void BeginFrame();
    void Submit(const WorldTransform& worldTransform, uint32_t color, uint32_t cloudNoiseSrvIndex, const AtmosphereSkyData& weather);
    void Draw(const RenderEnvironment& env, const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix);
private:
    static constexpr int kFrameCount = 3;
    int currentFrameIndex_ = 0;
    Mesh skydomeMesh_;
    MaterialHandle skydomeMaterialHandle_;
    // フレームごとのWVPバッファ
    Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource_[kFrameCount];
    TransformationMatrix* mappedWvp_[kFrameCount] = {};
    // フレームごとの天候バッファ
    Microsoft::WRL::ComPtr<ID3D12Resource> AtmosphereSkyResource_[kFrameCount];
    AtmosphereSkyData* mappedAtmosphereSky_[kFrameCount] = {};
    bool isSubmitted_ = false;
    WorldTransform currentTransform_;
    uint32_t currentColor_ = 0xFFFFFFFF;
    uint32_t cloudTextureIndex_ = 0;
    AtmosphereSkyData currentAtmosphereSkyData_; // Submitで受け取ったデータを保持
};

}