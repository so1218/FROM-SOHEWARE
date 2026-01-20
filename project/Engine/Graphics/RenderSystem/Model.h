#pragma once

#include <d3d12.h>
#include <string>

#include "WorldTransform.h"
#include "TextureHandle.h"

class Engine;

class Model
{
public:
    Model(Engine* engine, const ModelData* modelData);
    void Draw();

    // 複雑なセッター
    void SetUVTransform(const WorldTransform& uvTransform);

    // テクスチャ関係
    void SetTexture(TextureID textureID);
    void SetEnvironmentMapTexture(TextureID textureID);
    void SetToonRampTexture(TextureID textureID);
    void SetDissolveTexture(TextureID textureID);
    void SetNormalMapTexture(TextureID textureID);

    // 色変換関係 (Math依存)
    void SetColor(const Vector4& color);     
    void SetOutlineColor(uint32_t color);    

    // 単純なセッター
    void SetWorldTransform(const WorldTransform& transform) { transform_ = transform; }

    // 色の直接代入
    void SetColor(uint32_t color) { color_ = color; }
    void SetOutlineColor(const Vector4& color) { outlineColor_ = color; }

    // フラグ・パラメータ系
    void SetEnableOutline(bool enable) { enableOutline_ = enable; }
    void SetOutlineWidth(float width) { outlineWidth_ = width; }
    void SetRenderGroup(RenderGroup group) { renderGroup_ = group; }
    void SetBlendMode(BlendMode blendMode) { blendMode_ = blendMode; }

    // マテリアルパラメータ
    void SetEmissiveIntensity(float intensity) { materialHandle_.materialData->emissiveIntensity = intensity; }
    void SetEnableDissolve(bool enable) { materialHandle_.materialData->enableDissolve = enable; }

    // ゲッター
    WorldTransform& GetTransform() { return transform_; }
    const WorldTransform& GetTransform() const { return transform_; }

    WorldTransform& GetUVTransform() { return uvTransform_; };
    const WorldTransform& GetUVTransform() const { return uvTransform_; }

    uint32_t GetColor() const { return color_; }
    uint32_t* GetColorPtr() { return &color_; }

    bool IsOutlineEnabled() const { return enableOutline_; }
    float GetOutlineWidth() const { return outlineWidth_; }
    const Vector4& GetOutlineColor() const { return outlineColor_; }
    BlendMode GetBlendMode() const { return blendMode_; }

    // マテリアルデータへのアクセサ
    MaterialData* GetMaterial() { return materialHandle_.materialData; }
    const MaterialData* GetMaterial() const { return materialHandle_.materialData; }

private:
    Engine* engine_ = nullptr;
    const ModelData* modelData_;
    MaterialHandle materialHandle_;

    WorldTransform transform_;
    WorldTransform uvTransform_;

    // テクスチャハンドル
    uint32_t textureHandle_ = 0;
    uint32_t envMapTextureHandle_ = 0;
    uint32_t toonRampHandle_ = 0;
    uint32_t dissolveTextureHandle_ = 0;
    uint32_t normalMapHandle_ = 0;

    // 色・描画設定
    uint32_t color_ = 0xFFFFFFFF;
    BlendMode blendMode_ = BlendMode::kBlendModeNone;
    RenderGroup renderGroup_ = RenderGroup::Opaque;

    // アウトライン
    bool enableOutline_ = false;
    float outlineWidth_ = 7.0f;
    Vector4 outlineColor_ = { 0.0f, 0.0f, 0.0f, 1.0f };
};