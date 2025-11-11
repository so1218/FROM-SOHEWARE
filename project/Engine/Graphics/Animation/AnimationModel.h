#pragma once

#include "AnimationData.h" 
#include "Engine.h"
#include "Easing.h"

class AnimationModel
{
public:
    // コンストラクタ
    AnimationModel(Engine* engine, Camera* camera, ModelData modelData, Animation animation);

    // 更新処理
    void Update(float targetDuration, bool isLoop);

    // 描画処理
    void Draw();

    // アニメーションの状態をリセット
    void ResetAnimation();

    void SetEasing(EasingType type) { easingType_ = type; }

    // トランスフォームへの参照を取得
    WorldTransform& GetTransform() { return transform_; }

    void SetEnvironmentMapHandle(uint32_t handle);

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
};