#pragma once
#include "Mesh.h"
#include "RenderCommon.h"
#include "RenderEnvironment.h"

namespace FE
{

class SkyboxRenderer
{
public:
    void Initialize(const RenderEnvironment& env);
    void BeginFrame();

    void Submit(const WorldTransform& worldTransform, uint32_t color, uint32_t cubeTextureSrvIndex);

    // 描画時にカメラ行列を渡す
    void Draw(const RenderEnvironment& env, const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix);

private:
    Mesh skyboxMesh_;
    Microsoft::WRL::ComPtr<ID3D12Resource> skyboxWvpResource_;
    TransformationMatrix* mappedSkyboxWvp_ = nullptr;
    MaterialHandle skyboxMaterialHandle_;

    // 1フレーム分の描画情報
    bool isSubmitted_ = false;
    WorldTransform currentTransform_;
    uint32_t currentColor_ = 0xFFFFFFFF;
    uint32_t currentTextureIndex_ = 0;
};

}