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

    // 雲用のテクスチャインデックスを追加で受け取る
    void Submit(const WorldTransform& worldTransform, uint32_t color, uint32_t skyCubeSrvIndex, uint32_t cloudNoiseSrvIndex);

    void Draw(const RenderEnvironment& env, const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix);

private:
    Mesh skydomeMesh_;
    Microsoft::WRL::ComPtr<ID3D12Resource> skydomeWvpResource_;
    TransformationMatrix* mappedSkydomeWvp_ = nullptr;
    MaterialHandle skydomeMaterialHandle_;

    bool isSubmitted_ = false;
    WorldTransform currentTransform_;
    uint32_t currentColor_ = 0xFFFFFFFF;

    uint32_t skyTextureIndex_ = 0;
    uint32_t cloudTextureIndex_ = 0;
};

}