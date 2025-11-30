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
    void SetEnvironmentMapHandle(uint32_t handle);
    void SetToonRampHandle(uint32_t handle) { toonRampHandle_ = handle; }
    void SetColor(uint32_t color);
    void SetCamera(Camera* camera);
    void SetUVTransform(const WorldTransform& uvTransform);
    void SetEnableOutline(bool enable);
    void SetOutlineWidth(float width);
    void SetOutlineColor(const Vector4& color);
    void SetRenderGroup(RenderGroup group);

    // ゲッター
    WorldTransform& GetTransform() { return transform_; }
    WorldTransform& GetUVTransform() { return uvTransform_; };
    const WorldTransform& GetTransform() const { return transform_; }
    const WorldTransform& GetUVTransform() const { return uvTransform_; }
    uint32_t GetColor() const { return color_; }
    bool IsOutlineEnabled() const { return enableOutline_; }
    float GetOutlineWidth() const { return outlineWidth_; }
    const Vector4& GetOutlineColor() const { return outlineColor_; }
private:
    Engine* engine_ = nullptr;
    Camera* camera_ = nullptr;

    WorldTransform transform_;
    WorldTransform uvTransform_;

    uint32_t textureHandle_;
    uint32_t envMapTextureHandle_;
    uint32_t toonRampHandle_;
    uint32_t color_ = 0xFFFFFFFF;

    ModelData* modelData_;
    MaterialHandle materialHandle_;

    bool enableOutline_ = false;
    float outlineWidth_ = 7.0f; 
    Vector4 outlineColor_ = { 0.0f, 0.0f, 0.0f, 1.0f };

    RenderGroup renderGroup_ = RenderGroup::Opaque;

};