#pragma once

#include "AnimationData.h" 
#include "Engine.h"
#include "Easing.h"

class AnimationModel
{
public:
    // コンストラクタ
    AnimationModel(Engine* engine, Camera* camera, ModelData modelData, Animation animation);
    ~AnimationModel();

    // アニメーション更新
    void Update(float targetDuration, bool isLoop);

    // 描画処理
    void Draw();

    // アニメーションのリセット
    void ResetAnimation();

    // セッター
    void SetTransform(const WorldTransform& transform) { transform_ = transform; }
    void SetUvTransform(const WorldTransform& uvTransform) { uvTransform_ = uvTransform; }
    void SetColor(uint32_t color) { color_ = color; }
    void SetTextureHandle(uint32_t handle) { textureHandle_ = handle; }
    void SetMaterialHandle(MaterialHandle handle) { materialHandle_ = handle; }
    void SetEnvironmentMapHandle(uint32_t handle) { envMapTextureHandle_ = handle; }
    void SetEasing(EasingType type) { easingType_ = type; }
    // アウトライン設定
    void SetEnableOutline(bool enable);
    void SetOutline(float width, const Vector4& color) {
        outlineWidth_ = width;
        outlineColor_ = color;
    }
    void SetOutline(float width, uint32_t color) {
        outlineWidth_ = width;
        outlineColor_ = Math::Uint32ToColorVector(color); 
    }

    // ゲッター
    WorldTransform& GetTransform() { return transform_; }
    bool IsFinished() const { return isFinished_; }
    float GetAnimationTime() const { return animationTime_; }
    bool IsOutlineEnabled() const { return enableOutline_; }
    uint32_t GetColor() const { return color_; }

private:

    Engine* engine_ = nullptr;
    Camera* camera_ = nullptr;

    AnimatedModelData animeModelData_;
    WorldTransform transform_;
    WorldTransform uvTransform_;

    Skeleton skeleton_;
    SkinCluster skinCluster_;
    float animationTime_;

    uint32_t textureHandle_;
    uint32_t envMapTextureHandle_;
    uint32_t color_;
    MaterialHandle materialHandle_;

    bool isFinished_ = false;      // 再生が終了したか
    EasingType easingType_ = EasingType::EaseLinear;

    bool enableOutline_ = false;
    float outlineWidth_ = 5.0f;     
    Vector4 outlineColor_ = { 0.0f, 0.0f, 0.0f, 1.0f };
};