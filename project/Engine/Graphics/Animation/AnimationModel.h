#pragma once

#include "AnimationData.h" 
#include "AnimationHandle.h" 
#include "Engine.h"
#include "TextureHandle.h"

class AnimationModel
{
public:
    // コンストラクタ
    AnimationModel(Engine* engine, const ModelData* modelData, const Animation* animation);
    ~AnimationModel();

    // アニメーション更新
    void Update(float speedScale, bool isLoop);

    // 描画処理
    void Draw();

    // アニメーションのリセット
    void ResetAnimation();

    // 現在再生中のアニメーションのIDを取得(多重再生防止)
    AnimationID GetCurrentAnimationID() const { return currentAnimationID_; }

    // セッター
    void SetTransform(const WorldTransform& transform) { transform_ = transform; }
    void SetUvTransform(const WorldTransform& uvTransform) { uvTransform_ = uvTransform; }
    void SetColor(uint32_t color) { color_ = color; }
    void SetColor(const Vector4& color) { color_ = Math::ColorVectorToUint32(color); }
    void SetTexture(TextureID textureID) { textureHandle_ = TextureHandle::Get(textureID); }
    void SetMaterialHandle(MaterialHandle handle) { materialHandle_ = handle; }
    void SetEmissiveIntensity(float intensity) { materialHandle_.materialData->emissiveIntensity = intensity; }
    void SetEnvironmentMapTexture(TextureID textureID) { envMapTextureHandle_ = TextureHandle::Get(textureID); }
    void SetToonRampTexture(TextureID textureID) { toonRampHandle_ = TextureHandle::Get(textureID); }
    void SetEasing(EasingType type) { easingType_ = type; }
    void SetDissolveTexture(TextureID textureID) { dissolveTextureHandle_ = TextureHandle::Get(textureID); }
    void SetEnableDissolve(bool enable) { materialHandle_.materialData->enableDissolve = enable; }
    void SetNormalMapTexture(TextureID textureID) { normalMapHandle_ = TextureHandle::Get(textureID); }
    // アウトライン設定
    void SetEnableOutline(bool enable);
    void SetOutlineWidth(float width) { outlineWidth_ = width; }
    void SetOutlineColor(const Vector4& color) { outlineColor_ = color; }
    void SetOutlineColor(uint32_t color) { outlineColor_ = Math::Uint32ToColorVector(color); }

    void SetRenderGroup(RenderGroup group) { renderGroup_ = group; }

    // ゲッター
    WorldTransform& GetTransform() { return transform_; }
    bool IsFinished() const { return isFinished_; }
    float GetAnimationTime() const { return animationTime_; }
    bool IsOutlineEnabled() const { return enableOutline_; }
    uint32_t GetColor() const { return color_; }
    uint32_t* GetColorPtr() { return &color_; }

    // アニメーションを切り替える関数
    void SetAnimation(const Animation* animation);

    MaterialHandle materialHandle_;

private:

    Engine* engine_ = nullptr;

    AnimatedModelData animeModelData_;
    WorldTransform transform_;
    WorldTransform uvTransform_;

    Skeleton skeleton_;
    SkinCluster skinCluster_;
    float animationTime_;

    uint32_t textureHandle_;
    uint32_t envMapTextureHandle_;
    uint32_t toonRampHandle_;
    uint32_t normalMapHandle_;
    uint32_t color_;

    bool isFinished_ = false;      // 再生が終了したか
    EasingType easingType_ = EasingType::EaseLinear;

    bool enableOutline_ = false;
    float outlineWidth_ = 5.0f;
    Vector4 outlineColor_ = { 0.0f, 0.0f, 0.0f, 1.0f };

    RenderGroup renderGroup_ = RenderGroup::Opaque;

    AnimationID currentAnimationID_ = AnimationID::count;
    float speedScale_ = 1.0f;

    uint32_t dissolveTextureHandle_;
};