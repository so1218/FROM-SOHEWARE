#pragma once

#include <d3d12.h>
#include <string>

#include "WorldTransform.h"
#include "Camera.h"
#include "TextureManager.h"
#include "RenderCommon.h"
#include "LightManager.h"
#include "PSOManager.h"
#include "RootSignatureManager.h"
#include "MaterialManager.h"

class Engine;

class Model
{
public:
    Model(Engine* engine, Camera* camera, ModelData* modelData);

    void Draw();

    // セッター
    void SetWorldTransform(const WorldTransform& transform);
    void SetTextureHandle(uint32_t handle);
    void SetColor(uint32_t color);
    void SetCamera(Camera* camera);
    void SetUVTransform(const WorldTransform& uvTransform);

    // ゲッター
    WorldTransform& GetTransform() { return transform_; }
    WorldTransform& GetUVTransform() { return uvTransform_; };
    const WorldTransform& GetTransform() const { return transform_; }
    const WorldTransform& GetUVTransform() const { return uvTransform_; }
    uint32_t GetColor() const { return color_; }
    MaterialHandle materialHandle_;
private:
    Engine* engine_ = nullptr;
    Camera* camera_ = nullptr;

    WorldTransform transform_;
    WorldTransform uvTransform_;

    uint32_t textureHandle_ = 0;
    uint32_t color_ = 0xFFFFFFFF;

    ModelData* modelData_;

};