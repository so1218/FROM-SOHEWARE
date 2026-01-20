#pragma once

#include "AnimationData.h" 
#include "AnimationHandle.h" 
#include "TextureHandle.h"

class Engine;

class AnimationModel
{
public:
    AnimationModel(Engine* engine, const ModelData* modelData, const Animation* animation);
    ~AnimationModel();

    void Update(float speedScale, bool isLoop);
    void Draw();

    // アニメーション制御 
    void ResetAnimation();
    void SetAnimation(const Animation* animation);

    // 複雑なセッター
    // テクスチャ関係
    void SetTexture(TextureID textureID);
    void SetEnvironmentMapTexture(TextureID textureID);
    void SetToonRampTexture(TextureID textureID);
    void SetDissolveTexture(TextureID textureID);
    void SetNormalMapTexture(TextureID textureID);

    // 色変換関係
    void SetColor(const Vector4& color);       
    void SetOutlineColor(uint32_t color);      

    // 単純なセッター
    void SetTransform(const WorldTransform& transform) { transform_ = transform; }
    void SetUvTransform(const WorldTransform& uvTransform) { uvTransform_ = uvTransform; }

    // 色の直接代入
    void SetColor(uint32_t color) { color_ = color; }
    void SetOutlineColor(const Vector4& color) { outlineColor_ = color; }

    // フラグ・パラメータ系
    void SetEasing(EasingType type) { easingType_ = type; }
    void SetEnableOutline(bool enable) { enableOutline_ = enable; }
    void SetOutlineWidth(float width) { outlineWidth_ = width; }
    void SetRenderGroup(RenderGroup group) { renderGroup_ = group; }

    // マテリアルパラメータ
    void SetEmissiveIntensity(float intensity) { materialHandle_.materialData->emissiveIntensity = intensity; }
    void SetEnableDissolve(bool enable) { materialHandle_.materialData->enableDissolve = enable; }

    // マテリアルデータへのアクセサ
    MaterialData* GetMaterial() { return materialHandle_.materialData; }
    const MaterialData* GetMaterial() const { return materialHandle_.materialData; }

    // ゲッター
    WorldTransform& GetTransform() { return transform_; }
    AnimationID GetCurrentAnimationID() const { return currentAnimationID_; }
    bool IsFinished() const { return isFinished_; }
    float GetAnimationTime() const { return animationTime_; }

    bool IsOutlineEnabled() const { return enableOutline_; }
    uint32_t GetColor() const { return color_; }
    uint32_t* GetColorPtr() { return &color_; }

private:
    Engine* engine_ = nullptr;

    MaterialHandle materialHandle_;

    AnimatedModelData animeModelData_;
    WorldTransform transform_;
    WorldTransform uvTransform_;

    Skeleton skeleton_;
    SkinCluster skinCluster_;
    float animationTime_;

    // テクスチャハンドル
    uint32_t textureHandle_ = 0;
    uint32_t envMapTextureHandle_ = 0;
    uint32_t toonRampHandle_ = 0;
    uint32_t dissolveTextureHandle_ = 0;
    uint32_t normalMapHandle_ = 0;

    // 色情報
    uint32_t color_ = 0xFFFFFFFF;

    // アニメーション制御
    bool isFinished_ = false;       // 再生が終了したか
    EasingType easingType_ = EasingType::EaseLinear;
    AnimationID currentAnimationID_ = AnimationID::count;
    float speedScale_ = 1.0f;

    // アウトライン
    bool enableOutline_ = false;
    float outlineWidth_ = 5.0f;
    Vector4 outlineColor_ = { 0.0f, 0.0f, 0.0f, 1.0f };

    RenderGroup renderGroup_ = RenderGroup::Opaque;
};