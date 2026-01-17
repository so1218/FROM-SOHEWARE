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
    Model(Engine* engine, Camera* camera, const ModelData* modelData);

    void Draw();

    // セッター
    void SetWorldTransform(const WorldTransform& transform);
    void SetTextureHandle(uint32_t handle);
    void SetEnvironmentMapHandle(uint32_t handle);
    void SetToonRampHandle(uint32_t handle) { toonRampHandle_ = handle; }
    void SetColor(uint32_t color);
    void SetColor(const Vector4& color) { color_ = Math::ColorVectorToUint32(color); }
    void SetCamera(Camera* camera);
    void SetUVTransform(const WorldTransform& uvTransform);
    void SetEnableOutline(bool enable);
    void SetOutlineWidth(float width);
    void SetOutlineColor(uint32_t color) { outlineColor_ = Math::Uint32ToColorVector(color); }
    void SetOutlineColor(const Vector4& color);
    void SetRenderGroup(RenderGroup group);
    void SetEmissiveIntensity(float intensity) { materialHandle_.materialData->emissiveIntensity = intensity; }
    void SetBlendMode(BlendMode blendMode) { blendMode_ = blendMode; }
    void SetDissolveTextureHandle(uint32_t handle) { dissolveTextureHandle_ = handle; }
    void SetNormalMapHandle(uint32_t handle) { normalMapHandle_ = handle; }
    void SetEnableDissolve(bool enable) { materialHandle_.materialData->enableDissolve = enable; }

    // ゲッター
    WorldTransform& GetTransform() { return transform_; }
    WorldTransform& GetUVTransform() { return uvTransform_; };
    const WorldTransform& GetTransform() const { return transform_; }
    const WorldTransform& GetUVTransform() const { return uvTransform_; }
    uint32_t GetColor() const { return color_; }
    bool IsOutlineEnabled() const { return enableOutline_; }
    float GetOutlineWidth() const { return outlineWidth_; }
    const Vector4& GetOutlineColor() const { return outlineColor_; }
    BlendMode GetBlendMode() const { return blendMode_; }

    MaterialHandle materialHandle_;
private:
    Engine* engine_ = nullptr;
    Camera* camera_ = nullptr;

    WorldTransform transform_;
    WorldTransform uvTransform_;

    uint32_t textureHandle_;
    uint32_t envMapTextureHandle_;
    uint32_t toonRampHandle_;
    uint32_t normalMapHandle_;
    uint32_t color_ = 0xFFFFFFFF;
    BlendMode blendMode_ = BlendMode::kBlendModeNone;

    const ModelData* modelData_;

    bool enableOutline_ = false;
    float outlineWidth_ = 7.0f;
    Vector4 outlineColor_ = { 0.0f, 0.0f, 0.0f, 1.0f };

    RenderGroup renderGroup_ = RenderGroup::Opaque;

    uint32_t dissolveTextureHandle_;
};